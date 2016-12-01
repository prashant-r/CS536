//FILE: overlay/overlay.c
//
//Description: this file implements a ON process 
//A ON process first connects to all the neighbors and then starts listen_to_neighbor threads each of 
//which keeps receiving the incoming packets from a neighbor and forwarding the received packets to the 
//SNP process. Then ON process waits for the connection from SNP process. After a SNP process is connected,
// the ON process keeps receiving sendpkt_arg_t structures from the SNP process and sending the received 
//packets out to the overlay network. 
//
//Date: April 28, 2016
//Author: Victoria Taylor (skeleton code and main() provided by Prof. Xia Zhou)


#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <sys/utsname.h>
#include <assert.h>



// -> THe constants

//this is the port number that is used for nodes to interconnect each other to form an overlay, you should change this to a random value to avoid conflicts with other students
#define CONNECTION_PORT 3007

//this is the port number that is opened by overlay process and connected by the network layer process, you should change this to a random value to avoid conflicts with other students
#define OVERLAY_PORT 3666

//max packet data length
#define MAX_PKT_LEN 1488 


/*******************************************************************/
//network layer parameters
/*******************************************************************/
//max node number supported by the overlay network 
#define MAX_NODE_NUM 10

//infinite link cost value, if two nodes are disconnected, they will have link cost INFINITE_COST
#define INFINITE_COST 999

//the network layer process opens this port, and waits for connection from transport layer process, you should change this to a random value to avoid conflicts with other students
#define NETWORK_PORT 4002

//this is the broadcasting nodeID. If the overlay layer process receives a packet destined to BROADCAST_NODEID from the network layer process, it should send this packet to all the neighbors
#define BROADCAST_NODEID 9999

//route update broadcasting interval in seconds
#define ROUTEUPDATE_INTERVAL 5





// -> neighbor table.h stuff
//neighbor table entry definition
//a neighbor table contains n entries where n is the number of neighbors
//Each Node has a Overlay Network (ON) process running, each ON process maintains the neighbor table for the node that the process is running on.

typedef struct neighborentry {
  int nodeID;	        //neighbor's node ID
  in_addr_t nodeIP;     //neighbor's IP address
  int conn;	        //TCP connection's socket descriptor to the neighbor
} nbr_entry_t;


//This function first creates a neighbor table dynamically. It then parses the topology/topology.dat file and fill the nodeID and nodeIP fields in all the entries, initialize conn field as -1 .
//return the created neighbor table
nbr_entry_t* nt_create();

//This function destroys a neighbortable. It closes all the connections and frees all the dynamically allocated memory.
void nt_destroy(nbr_entry_t* nt);

//This function is used to assign a TCP connection to a neighbor table entry for a neighboring node. If the TCP connection is successfully assigned, return 1, otherwise return -1
int nt_addconn(nbr_entry_t* nt, int nodeID, int conn);


// -> pkt.h constants


//packet type definition, used for type field in packet header
#define	ROUTE_UPDATE 1
#define SNP 2	

//SNP packet format definition
typedef struct snpheader {
  int src_nodeID;		          //source node ID
  int dest_nodeID;		        //destination node ID
  unsigned short int length;	//length of the data in the packet
  unsigned short int type;	  //type of the packet 
} snp_hdr_t;

typedef struct packet {
  snp_hdr_t header;
  char data[MAX_PKT_LEN];
} snp_pkt_t;


//route update packet definition
//for a route update packet, the route update information will be stored in the data field of a packet 

//a route update entry
typedef struct routeupdate_entry {
        unsigned int nodeID;	//destination nodeID
        unsigned int cost;	//link cost from the source node(src_nodeID in header) to destination node
} routeupdate_entry_t;

//route update packet format
typedef struct pktrt{
        unsigned int entryNum;	//number of entries contained in this route update packet
        routeupdate_entry_t entry[MAX_NODE_NUM];
} pkt_routeupdate_t;



// sendpkt_arg_t data structure is used in the overlay_sendpkt() function. 
// overlay_sendpkt() is called by the SNP process to request 
// the ON process to send a packet out to the overlay network. 
// 
// The ON process and SNP process are connected with a 
// local TCP connection, in overlay_sendpkt(), the SNP process 
// sends this data structure over this TCP connection to the ON process. 
// The ON process receives this data structure by calling getpktToSend().
// Then the ON process sends the packet out to the next hop by calling sendpkt().
typedef struct sendpktargument {
  int nextNodeID;    //node ID of the next hop
  snp_pkt_t pkt;         //the packet to be sent
} sendpkt_arg_t;


// overlay_sendpkt() is called by the SNP process to request the ON 
// process to send a packet out to the overlay network. The 
// ON process and SNP process are connected with a local TCP connection. 
// In overlay_sendpkt(), the packet and its next hop's nodeID are encapsulated 
// in a sendpkt_arg_t data structure and sent over this TCP connection to the ON process. 
// The parameter overlay_conn is the TCP connection's socket descriptior 
// between the SNP process and the ON process.
// When sending the sendpkt_arg_t data structure over the TCP connection between the SNP 
// process and the ON process, use '!&'  and '!#' as delimiters. 
// Send !& sendpkt_arg_t structure !# over the TCP connection.
// Return 1 if sendpkt_arg_t data structure is sent successfully, otherwise return -1.
int overlay_sendpkt(int nextNodeID, snp_pkt_t* pkt, int overlay_conn);




// overlay_recvpkt() function is called by the SNP process to receive a packet 
// from the ON process. The parameter overlay_conn is the TCP connection's socket 
// descriptior between the SNP process and the ON process. The packet is sent over 
// the TCP connection between the SNP process and the ON process, and delimiters 
// !& and !# are used. 
// To receive the packet, this function uses a simple finite state machine(FSM)
// PKTSTART1 -- starting point 
// PKTSTART2 -- '!' received, expecting '&' to receive data 
// PKTRECV -- '&' received, start receiving data
// PKTSTOP1 -- '!' received, expecting '#' to finish receiving data 
// Return 1 if a packet is received successfully, otherwise return -1.
int overlay_recvpkt(snp_pkt_t* pkt, int overlay_conn);



// This function is called by the ON process to receive a sendpkt_arg_t data structure.
// A packet and the next hop's nodeID is encapsulated  in the sendpkt_arg_t structure.
// The parameter network_conn is the TCP connection's socket descriptior between the
// SNP process and the ON process. The sendpkt_arg_t structure is sent over the TCP 
// connection between the SNP process and the ON process, and delimiters !& and !# are used. 
// To receive the packet, this function uses a simple finite state machine(FSM)
// PKTSTART1 -- starting point 
// PKTSTART2 -- '!' received, expecting '&' to receive data 
// PKTRECV -- '&' received, start receiving data
// PKTSTOP1 -- '!' received, expecting '#' to finish receiving data
// Return 1 if a sendpkt_arg_t structure is received successfully, otherwise return -1.
int getpktToSend(snp_pkt_t* pkt, int* nextNode,int network_conn);




// forwardpktToSNP() function is called after the ON process receives a packet from 
// a neighbor in the overlay network. The ON process calls this function 
// to forward the packet to SNP process. 
// The parameter network_conn is the TCP connection's socket descriptior between the SNP 
// process and ON process. The packet is sent over the TCP connection between the SNP process 
// and ON process, and delimiters !& and !# are used. 
// Send !& packet data !# over the TCP connection.
// Return 1 if the packet is sent successfully, otherwise return -1.
int forwardpktToSNP(snp_pkt_t* pkt, int network_conn);



// sendpkt() function is called by the ON process to send a packet 
// received from the SNP process to the next hop.
// Parameter conn is the TCP connection's socket descritpor to the next hop node.
// The packet is sent over the TCP connection between the ON process and a neighboring node,
// and delimiters !& and !# are used. 
// Send !& packet data !# over the TCP connection
// Return 1 if the packet is sent successfully, otherwise return -1.
int sendpkt(snp_pkt_t* pkt, int conn);



// recvpkt() function is called by the ON process to receive 
// a packet from a neighbor in the overlay network.
// Parameter conn is the TCP connection's socket descritpor to a neighbor.
// The packet is sent over the TCP connection  between the ON process and the neighbor,
// and delimiters !& and !# are used. 
// To receive the packet, this function uses a simple finite state machine(FSM)
// PKTSTART1 -- starting point 
// PKTSTART2 -- '!' received, expecting '&' to receive data 
// PKTRECV -- '&' received, start receiving data
// PKTSTOP1 -- '!' received, expecting '#' to finish receiving data 
// Return 1 if the packet is received successfully, otherwise return -1.
int recvpkt(snp_pkt_t* pkt, int conn);




/// -> Packet stuff
// overlay_sendpkt() is called by the SNP process to request the ON 
// process to send a packet out to the overlay network. The 
// ON process and SNP process are connected with a local TCP connection. 
// In overlay_sendpkt(), the packet and its next hop's nodeID are encapsulated 
// in a sendpkt_arg_t data structure and sent over this TCP connection to the ON process. 
// The parameter overlay_conn is the TCP connection's socket descriptior 
// between the SNP process and the ON process.
// When sending the sendpkt_arg_t data structure over the TCP connection between the SNP 
// process and the ON process, use '!&'  and '!#' as delimiters. 
// Send !& sendpkt_arg_t structure !# over the TCP connection.
// Return 1 if sendpkt_arg_t data structure is sent successfully, otherwise return -1.
int overlay_sendpkt(int nextNodeID, snp_pkt_t* pkt, int overlay_conn)
{
	// Set up sendpkt_arg_t
	sendpkt_arg_t packet; 
	packet.nextNodeID = nextNodeID;
	packet.pkt = *pkt;

	//set up buf start and buf end
	char bufstart[2];
	char bufend[2];
	bufstart[0] = '!';
	bufstart[1] = '&';
	bufend[0] = '!';
	bufend[1] = '#';

	if (send(overlay_conn, bufstart, 2, 0) < 0){
		return -1;
	}
	if (send(overlay_conn, &packet, sizeof(sendpkt_arg_t), 0) < 0){
		return -1;
	}
	if (send(overlay_conn, bufend, 2, 0) < 0){
		return -1;
	}

	return 1; 
}


// overlay_recvpkt() function is called by the SNP process to receive a packet 
// from the ON process. The parameter overlay_conn is the TCP connection's socket 
// descriptior between the SNP process and the ON process. The packet is sent over 
// the TCP connection between the SNP process and the ON process, and delimiters 
// !& and !# are used. 
// To receive the packet, this function uses a simple finite state machine(FSM)
// 0  PKTSTART1 -- starting point 
// 1 PKTSTART2 -- '!' received, expecting '&' to receive data 
// 2 PKTRECV -- '&' received, start receiving data
// 3 PKTSTOP1 -- '!' received, expecting '#' to finish receiving data 
// 4 Return 1 if a packet is received successfully, otherwise return -1.
int overlay_recvpkt(snp_pkt_t* pkt, int overlay_conn)
{
	char buf[sizeof(snp_pkt_t)+2]; 
	char c;
	int idx = 0;

	int state = 0; 
	while(recv(overlay_conn,&c,1,0)>0) {
		if (state == 0) {
		        if(c=='!')
				state = 1;
		}
		else if(state == 1) {
			if(c=='&') 
				state = 2;
			else
				state = 0;
		}
		else if(state == 2) {
			if(c=='!') {
				buf[idx]=c;
				idx++;
				state = 3;
			}
			else {
				buf[idx]=c;
				idx++;
			}
		}
		else if(state == 3) {
			if(c=='#') {
				buf[idx]=c;
				idx++;
				state = 0;
				idx = 0;
				memcpy(pkt,buf,sizeof(snp_pkt_t));
				return 1;
			}
			else if(c=='!') {
				buf[idx]=c;
				idx++;
			}
			else {
				buf[idx]=c;
				idx++;
				state = 2;
			}
		}
	}
	return -1;
}



// This function is called by the ON process to receive a sendpkt_arg_t data structure.
// A packet and the next hop's nodeID is encapsulated  in the sendpkt_arg_t structure.
// The parameter network_conn is the TCP connection's socket descriptior between the
// SNP process and the ON process. The sendpkt_arg_t structure is sent over the TCP 
// connection between the SNP process and the ON process, and delimiters !& and !# are used. 
// To receive the packet, this function uses a simple finite state machine(FSM)
// PKTSTART1 -- starting point 
// PKTSTART2 -- '!' received, expecting '&' to receive data 
// PKTRECV -- '&' received, start receiving data
// PKTSTOP1 -- '!' received, expecting '#' to finish receiving data
// Return 1 if a sendpkt_arg_t structure is received successfully, otherwise return -1.
int getpktToSend(snp_pkt_t* pkt, int* nextNode,int network_conn)
{
	sendpkt_arg_t receivearg;
	char buf[sizeof(sendpkt_arg_t)+2]; 
	char c;
	int idx = 0;

	int state = 0; 
	while(recv(network_conn,&c,1,0)>0) {
		if (state == 0) {
		        if(c=='!')
				state = 1;
		}
		else if(state == 1) {
			if(c=='&') 
				state = 2;
			else
				state = 0;
		}
		else if(state == 2) {
			if(c=='!') {
				buf[idx]=c;
				idx++;
				state = 3;
			}
			else {
				buf[idx]=c;
				idx++;
			}
		}
		else if(state == 3) {
			if(c=='#') {
				buf[idx]=c;
				idx++;
				state = 0;
				idx = 0;
				memcpy(&receivearg,buf,sizeof(sendpkt_arg_t));
				*pkt = receivearg.pkt;
				*nextNode = receivearg.nextNodeID;
				return 1;
			}
			else if(c=='!') {
				buf[idx]=c;
				idx++;
			}
			else {
				buf[idx]=c;
				idx++;
				state = 2;
			}
		}
	}
	return -1;
}




// forwardpktToSNP() function is called after the ON process receives a packet from 
// a neighbor in the overlay network. The ON process calls this function 
// to forward the packet to SNP process. 
// The parameter network_conn is the TCP connection's socket descriptior between the SNP 
// process and ON process. The packet is sent over the TCP connection between the SNP process 
// and ON process, and delimiters !& and !# are used. 
// Send !& packet data !# over the TCP connection.
// Return 1 if the packet is sent successfully, otherwise return -1.
int forwardpktToSNP(snp_pkt_t* pkt, int network_conn)
{
	char bufstart[2];
	char bufend[2];
	bufstart[0] = '!';
	bufstart[1] = '&';
	bufend[0] = '!';
	bufend[1] = '#';

	if (send(network_conn, bufstart, 2, 0) < 0) {
		return -1;
	}
	if(send(network_conn,pkt,sizeof(snp_pkt_t),0)<0) {
		return -1;
	}
	if(send(network_conn,bufend,2,0)<0) {
		return -1;
	}
	return 1; 

}



// sendpkt() function is called by the ON process to send a packet 
// received from the SNP process to the next hop.
// Parameter conn is the TCP connection's socket descritpor to the next hop node.
// The packet is sent over the TCP connection between the ON process and a neighboring node,
// and delimiters !& and !# are used. 
// Send !& packet data !# over the TCP connection
// Return 1 if the packet is sent successfully, otherwise return -1.
int sendpkt(snp_pkt_t* pkt, int conn)
{
	char bufstart[2];
	char bufend[2];
	bufstart[0] = '!';
	bufstart[1] = '&';
	bufend[0] = '!';
	bufend[1] = '#';

	if (send(conn, bufstart, 2, 0) < 0) {
		return -1;
	}
	if(send(conn,pkt,sizeof(snp_pkt_t),0)<0) {
		return -1;
	}
	if(send(conn,bufend,2,0)<0) {
		return -1;
	}
	return 1; 

}



// recvpkt() function is called by the ON process to receive 
// a packet from a neighbor in the overlay network.
// Parameter conn is the TCP connection's socket descritpor to a neighbor.
// The packet is sent over the TCP connection  between the ON process and the neighbor,
// and delimiters !& and !# are used. 
// To receive the packet, this function uses a simple finite state machine(FSM)
// PKTSTART1 -- starting point 
// PKTSTART2 -- '!' received, expecting '&' to receive data 
// PKTRECV -- '&' received, start receiving data
// PKTSTOP1 -- '!' received, expecting '#' to finish receiving data 
// Return 1 if the packet is received successfully, otherwise return -1.
int recvpkt(snp_pkt_t* pkt, int conn)
{
	char buf[sizeof(snp_pkt_t)+2]; 
	char c;
	int idx = 0;

	int state = 0; 
	while(recv(conn,&c,1,0)>0) {
		if (state == 0) {
		        if(c=='!')
				state = 1;
		}
		else if(state == 1) {
			if(c=='&') 
				state = 2;
			else
				state = 0;
		}
		else if(state == 2) {
			if(c=='!') {
				buf[idx]=c;
				idx++;
				state = 3;
			}
			else {
				buf[idx]=c;
				idx++;
			}
		}
		else if(state == 3) {
			if(c=='#') {
				buf[idx]=c;
				idx++;
				state = 0;
				idx = 0;
				memcpy(pkt,buf,sizeof(snp_pkt_t));
				return 1;
			}
			else if(c=='!') {
				buf[idx]=c;
				idx++;
			}
			else {
				buf[idx]=c;
				idx++;
				state = 2;
			}
		}
	}
	return -1;
}



// -> topology stuff
//this function returns node ID of the given hostname
//the node ID is an integer of the last 8 digit of the node's IP address
//for example, a node with IP address 202.120.92.3 will have node ID 3
//if the node ID can't be retrieved, return -1
int topology_getNodeIDfromname(char* hostname) 
{
	int nodeID;
	struct sockaddr_in addr; 

	struct hostent *host = gethostbyname(hostname);
	if (host == NULL){
		return -1;
	}
	memcpy(&(addr.sin_addr), host->h_addr_list[0], host->h_length);
	nodeID = topology_getNodeIDfromip(&addr.sin_addr);

	return nodeID;
}

in_addr_t topology_getIPfromname(char* hostname){
	struct hostent *host;
	struct sockaddr_in addr; 

	host = gethostbyname(hostname);

	memcpy(&(addr.sin_addr), host->h_addr_list[0], host->h_length);
	return addr.sin_addr.s_addr;

}

//this function returns node ID from the given IP address
//if the node ID can't be retrieved, return -1
int topology_getNodeIDfromip(struct in_addr* addr)
{
	strtok(inet_ntoa(*addr), ".");
	strtok(NULL, ".");
	strtok(NULL, ".");
	char *ip = strtok(NULL, ".");
	if (ip != NULL) {
		return atoi(ip);
	}
	else {
		return -1; 
	}
	
}

//this function returns my node ID
//if my node ID can't be retrieved, return -1
int topology_getMyNodeID()
{
	int myID;
	char hostname[50];
	if (gethostname(hostname, 50) < 0 ){
		return -1;
	}
	myID = topology_getNodeIDfromname(hostname);

	return myID; 
}

//this functions parses the topology information stored in topology.dat
//returns the number of neighbors
int topology_getNbrNum()
{
	char line[100];
	char *hostone, *hosttwo;
	int neighbors = 0; 
	int myID = topology_getMyNodeID();

	// Open topology.dat
	FILE *fd = fopen("config.dat", "r");
	if (fd == NULL){
		printf("File can't be opened\n");
		return -1;
	}

	// Count neighbors
	while (fgets(line, 100, fd) != NULL){
		if (line[0] != '\n'){
			hostone = strtok(line, " ");
			hosttwo = strtok(NULL, " ");
			if ((topology_getNodeIDfromname(hostone) == myID) || (topology_getNodeIDfromname(hosttwo)) == myID){
				neighbors++;
			}
		}
	}

	//Close file and return number of neighbors
	fclose(fd);
	return neighbors;
}

//this functions parses the topology information stored in topology.dat
//returns the number of total nodes in the overlay 
int topology_getNodeNum()
{
	int nodes[50] = {0};
	int nodecount;
	int oneInTable, twoInTable;
	char *hostone, *hosttwo;
	int oneID, twoID;
	FILE *fp; 
	char line[100];

	//Open topology.dat
	fp = fopen("config.dat", "r");
	if (fp == NULL){
		printf("Could not open config.dat\n");
		return -1; 
	}

	// parse each line and get the nodeIDs
	nodecount = 0;
	while (fgets(line, 100, fp) != NULL){
		if (line[0] != '\n'){
			
			//Pull node IDs from topology.dat
			hostone = strtok(line, " ");
			hosttwo = strtok(NULL, " ");
			oneID = topology_getNodeIDfromname(hostone); 
			twoID = topology_getNodeIDfromname(hosttwo);
			
			//Reset flags
			oneInTable = 0; 
			twoInTable = 0;

			// Check if nodes are already in the list
			int i = 0;
			while (nodes[i] != 0){
				if (nodes[i] == oneID){
					oneInTable = 1;
				}
				if (nodes[i] == twoID){
					twoInTable = 1;
				}
				i++;
			}

			//If nodes aren't in list- add them and update count
			if (!oneInTable){
				nodes[nodecount] = oneID;
				nodecount++;
			}
			if (!twoInTable){
				nodes[nodecount] = twoID;
				nodecount++;
			}
		}
	}

	//return total number of nodes
	fclose(fp);
	return nodecount;
}



//this functions parses the topology information stored in topology.dat
//returns the cost of the direct link between the two given nodes 
//if no direct link between the two given nodes, INFINITE_COST is returned
unsigned int topology_getCost(int fromNodeID, int toNodeID)
{
	char *hostone, *hosttwo,  *cost;
	int oneID, twoID;
	char line[100];

	FILE *fp = fopen("../topology/topology.dat", "r");
	if (fp == NULL){
		printf("Could not open topology.dat\n");
		return -1; 
	}

	while (fgets(line, 100, fp) != NULL){
		if (line[0] != '\n'){
			hostone = strtok(line, " ");
			hosttwo = strtok(NULL, " ");
			cost = strtok(NULL, " ");

			printf("hostone = %s, hosttwo= %s\n", hostone, hosttwo);
			oneID = topology_getNodeIDfromname(hostone);
			twoID = topology_getNodeIDfromname(hosttwo);

			//Return the cost of a direct link
			if (((oneID == fromNodeID) && (twoID == toNodeID)) || ((oneID == toNodeID) && (twoID == fromNodeID))){
				return atoi(cost); 
			}
		}
	}

	//No match found
	return INFINITE_COST;
}



// -> overlay.c


//you should start the ON processes on all the overlay hosts within this period of time
#define OVERLAY_START_DELAY 25


// This thread opens a TCP port on CONNECTION_PORT and waits for the incoming connection from all the neighbors that have a larger node ID than my nodeID,
// After all the incoming connections are established, this thread terminates 
void* waitNbrs();

// This function connects to all the neighbors that have a smaller node ID than my nodeID
// After all the outgoing connections are established, return 1, otherwise return -1
int connectNbrs();


//This function opens a TCP port on OVERLAY_PORT, and waits for the incoming connection from local SNP process. After the local SNP process is connected, this function keeps getting send_arg_t structures from SNP process, and sends the packets to the next hop in the overlay network.
void waitNetwork();

//Each listen_to_neighbor thread keeps receiving packets from a neighbor. It handles the received packets by forwarding the packets to the SNP process.
//all listen_to_neighbor threads are started after all the TCP connections to the neighbors are established 
void* listen_to_neighbor(void* arg);

//this function stops the overlay
//it closes all the connections and frees all the dynamically allocated memory
//it is called when receiving a signal SIGINT
void overlay_stop(); 

/**************************************************************/
//declare global variables
/**************************************************************/

//declare the neighbor table as global variable 
nbr_entry_t* nt; 
//declare the TCP connection to SNP process as global variable
int network_conn; 


/**************************************************************/
//implementation overlay functions
/**************************************************************/

// This thread opens a TCP port on CONNECTION_PORT and waits for the incoming connection from all the neighbors that have a larger node ID than my nodeID,
// After all the incoming connections are established, this thread terminates 
void* waitNbrs(void* arg) {
	// create TCP port on connection port
	int sd, connection;  
	int ntoConnect; 
	int myID, neighborNum, newnodeID;
	struct sockaddr_in myaddr, connectedaddr;
	socklen_t connaddrlen;
	connaddrlen = sizeof(connectedaddr);

	//Find number of neighbors with larger node ID
	myID = topology_getMyNodeID();
	neighborNum = topology_getNbrNum();
	ntoConnect = 0;
	int i;
	for (i = 0; i < neighborNum; i++){
		if (nt[i].nodeID > myID){
			ntoConnect++;
		}
	}

	//Set up server address
	memset(&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(CONNECTION_PORT);

	//Initialize socket, bind, and listen
	sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd < 0){
		printf("err: socket creation failed\n");
		return NULL;
	}
	if (bind(sd, (struct sockaddr *)&myaddr, sizeof(myaddr))<0){
		printf("err: bind failed\n");
		return NULL;
	}
	if (listen(sd, MAX_NODE_NUM) < 0){
		printf("err: listen failed\n");
		return NULL;
	}

	// Accept connections until all larger neighbors have connected
	printf("Connections to receive: %d\n", ntoConnect);
	while (ntoConnect > 0){
		connection = accept(sd, (struct sockaddr*)&connectedaddr, &connaddrlen);
		if (connection == -1){
			printf("err: connection failed\n");
			continue; 
		}

		newnodeID = topology_getNodeIDfromip(&connectedaddr.sin_addr);
		printf("Received conn from node = %d\n", newnodeID);
		nt_addconn(nt, newnodeID, connection);
		ntoConnect--;
	}
	return NULL;
}

// This function connects to all the neighbors that have a smaller node ID than my nodeID
// After all the outgoing connections are established, return 1, otherwise return -1
int connectNbrs() {
	int neighborNum, myID;
	int newconn; 
	struct sockaddr_in servaddr;

	neighborNum = topology_getNbrNum();
	myID = topology_getMyNodeID();

	int i;
	for (i = 0; i < neighborNum; i++){
		if (nt[i].nodeID < myID){

			//Set up server address
			memset(&servaddr, 0, sizeof(servaddr));
			servaddr.sin_port = htons(CONNECTION_PORT);
			servaddr.sin_addr.s_addr = nt[i].nodeIP;
			servaddr.sin_family = AF_INET;


			// Set up socket and connect
			newconn = socket(AF_INET, SOCK_STREAM, 0);
			if (newconn < 0){
				printf("err: can't create the socket\n");
				return -1; 
			}

			if (connect(newconn, (struct sockaddr*)&servaddr, sizeof(servaddr))<0){
				printf("err: connection failed\n");
				return -1;
			}
			printf("Connection est to %d\n", nt[i].nodeID);

			//create the new connection in the table
			nt_addconn(nt, nt[i].nodeID, newconn);
		}
	}
  return 1; 
}

//Each listen_to_neighbor thread keeps receiving packets from a neighbor. It handles the received packets by forwarding the packets to the SNP process.
//all listen_to_neighbor threads are started after all the TCP connections to the neighbors are established 
void* listen_to_neighbor(void* arg) {
	int index;
	int conn;
	snp_pkt_t pkt; 

	// Cast arg back into an int
	index = *(int*) arg;

	//find the connection in that index
	conn = nt[index].conn;

	//Continually receive packets from neighbor and forward to SNP 
	while (recvpkt(&pkt, conn) > 0){
		printf("Received packet from neighbor %d\n", nt[index].nodeID);

		if (forwardpktToSNP(&pkt, network_conn) > 0){
			printf("Forwarded packet to SNP\n");
		}
	}
	close(conn);
	nt[index].conn = -1;
	pthread_exit(0);
}

/*This function opens a TCP port on OVERLAY_PORT, and waits for the 
incoming connection from local SNP process. After the local SNP process is 
connected, this function keeps getting sendpkt_arg_ts from SNP process, and 
sends the packets to the next hop in the overlay network. If the next hop's 
nodeID is BROADCAST_NODEID, the packet should be sent to all the neighboring 
nodes.
*/
void waitNetwork() {
	int overlayserver;
	struct sockaddr_in serv_addr, snp_addr;
	socklen_t snp_addr_len;
	snp_addr_len = sizeof(snp_addr);
	snp_pkt_t pkt;
	int nextNode;

	int numNeighbors = topology_getNbrNum();

	//Create listening socket
	overlayserver = socket(AF_INET, SOCK_STREAM, 0);
	if (overlayserver < 0){
		printf("err: socket creation failed for ON\n");
		return;
	}

	//Set up server address

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(OVERLAY_PORT);

	//bind socket and listen for connection
	if (bind(overlayserver, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
		printf("err: bind failed for ON server\n");
		return;
	}
	if (listen(overlayserver, 1) < 0){
		printf("err: listen failed\n");
		return;
	}

	while (1){
		network_conn = accept(overlayserver, (struct sockaddr*)&snp_addr, &snp_addr_len);
		if (network_conn == -1){
			printf("Connection could not be est with SNP\n");
			continue;
		}
		printf("SNP connection est\n");

		while (getpktToSend(&pkt, &nextNode, network_conn) > 0){
			printf("Received packet from SNP\n");


			// If next node is BROADCAST_NODEID send to all the neighbors
			if (nextNode == BROADCAST_NODEID){
				int i ;
				for (i = 0; i < numNeighbors; i++){
					if (nt[i].conn != -1){
						sendpkt(&pkt, nt[i].conn);
						printf("Forwarded packet to %d\n", nt[i].nodeID);
					}
				}
			}

			// Otherwise find the next node in the table
			else {
				int i;
				for (i = 0; i < numNeighbors; i++){
					if (nt[i].nodeID == nextNode){
						if (sendpkt(&pkt, nt[i].conn) < 0){
							printf("failed to forward packet\n");
						}
						else{
							printf("forwarded packet\n");
						}
						break;
					}
				}
			}
		}
		close(network_conn);
		network_conn = -1;
	}
}

//this function stops the overlay
//it closes all the connections and frees all the dynamically allocated memory
//it is called when receiving a signal SIGINT
void overlay_stop() {
	//Close all overlay connections and free neighbortable
	nt_destroy(nt);

	//Close SNP connection
	close(network_conn);
	network_conn = -1;

	exit(0);
}

int main() {

	//start overlay initialization
	printf("Overlay: Node %d initializing...\n",topology_getMyNodeID());	

	//create a neighbor table
	nt = nt_create();
	//initialize network_conn to -1, means no SNP process is connected yet
	network_conn = -1;
	
	//register a signal handler which is sued to terminate the process
	signal(SIGINT, overlay_stop);

	//print out all the neighbors
	int nbrNum = topology_getNbrNum();
	int i;
	for(i=0;i<nbrNum;i++) {
		printf("Overlay: neighbor %d:%d\n",i+1,nt[i].nodeID);
	}

	//start the waitNbrs thread to wait for incoming connections from neighbors with larger node IDs
	pthread_t waitNbrs_thread;
	pthread_create(&waitNbrs_thread,NULL,waitNbrs,(void*)0);

	//wait for other nodes to start
	sleep(OVERLAY_START_DELAY);
	
	//connect to neighbors with smaller node IDs
	connectNbrs();

	//wait for waitNbrs thread to return
	pthread_join(waitNbrs_thread,NULL);	

	//at this point, all connections to the neighbors are created
	
	//create threads listening to all the neighbors
	for(i=0;i<nbrNum;i++) {
		int* idx = (int*)malloc(sizeof(int));
		*idx = i;
		pthread_t nbr_listen_thread;
		pthread_create(&nbr_listen_thread,NULL,listen_to_neighbor,(void*)idx);
	}
	printf("Overlay: node initialized...\n");
	printf("Overlay: waiting for connection from SNP process...\n");

	//waiting for connection from  SNP process
	waitNetwork();
}


// -> neighbor table stuff


//This function first creates a neighbor table dynamically. It then parses the topology/topology.dat file and fill the nodeID and nodeIP fields in all the entries, initialize conn field as -1 .
//return the created neighbor table
nbr_entry_t* nt_create()
{
	int neighborNum;
	nbr_entry_t *neighborTable;
	char line[100];
	char *hostone, *hosttwo; 
	int oneID, twoID, myID;
	int count;

	// Dynamically allocate neighbor table
	neighborNum = topology_getNbrNum();
	myID = topology_getMyNodeID();

	if ((neighborTable = malloc(sizeof(nbr_entry_t) * neighborNum)) == NULL){
		printf("malloc of neighbortable failed\n");
		exit(1);
	}

	// Open topology.dat
	FILE *fd = fopen("config.dat", "r");
	if (fd == NULL){
		printf("Can't open config.dat\n");
		return NULL;
	}

	//Parse topology.dat
	count = 0;
	while (fgets(line, 100, fd) != NULL){
		if (line[0] != '\n'){
			hostone = strtok(line, " ");
			hosttwo = strtok(NULL, " ");

			oneID = topology_getNodeIDfromname(hostone);
			twoID = topology_getNodeIDfromname(hosttwo);

			
			
			//Check if first host is equal to myID
			if (oneID == myID){
				neighborTable[count].nodeID = twoID;
				neighborTable[count].nodeIP = topology_getIPfromname(hosttwo);
				neighborTable[count].conn = -1;
				count++;
			}
			//Check if second host is equal to myID
			else if (twoID == myID){
				neighborTable[count].nodeID = oneID;
				neighborTable[count].nodeIP = topology_getIPfromname(hostone);
				neighborTable[count].conn = -1; 
				count++;
			}
		}
	}

  return neighborTable;
}

//This function destroys a neighbortable. It closes all the connections and frees all the dynamically allocated memory.
void nt_destroy(nbr_entry_t* nt)
{
	int neighborNum = topology_getNbrNum();

	//Close all TCP connections to neighbors
	int i;
	for (i = 0; i < neighborNum; i++){
		close(nt[i].conn);
	}

	//Free neighbortable
	free(nt);
  	return;
}

//This function is used to assign a TCP connection to a neighbor table entry for a neighboring node. If the TCP connection is successfully assigned, return 1, otherwise return -1
int nt_addconn(nbr_entry_t* nt, int nodeID, int conn)
{
	int neighborNum = topology_getNbrNum();
	int i;
	for (i = 0; i < neighborNum; i++){
		if (nt[i].nodeID == nodeID){
			// Make sure connection has not already been assigned for the neighbor
			if (nt[i].conn != -1){
				printf("Connection already assigned for this neighbor\n");
				return -1;
			}

			// If no connection has been established, assign the passed connection
			nt[i].conn = conn; 
			return 1; 
		}
	}
  return -1; 
}
