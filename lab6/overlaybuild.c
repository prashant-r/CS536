#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netdb.h>
#include <assert.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <pthread.h>
#include <unistd.h>

//max packet data length
#define MAX_PKT_LEN 1488 

//this is the broadcasting nodeID. If the overlay layer process receives a packet destined to BROADCAST_NODEID from the network layer process, it should send this packet to all the neighbors
#define BROADCAST_NODEID 9999

//this is the port number that is opened by overlay process and connected by the network layer process, you should change this to a random value to avoid conflicts with other students
#define OVERLAY_PORT 3666

//route update broadcasting interval in seconds
#define ROUTEUPDATE_INTERVAL 5



//SNP packet format definition
typedef struct snpheader {
  int src_nodeID;		          //source node ID
  int dest_nodeID;		        //destination node ID
  unsigned short int length;	//length of the data in the packet
  unsigned short int type;	  //type of the packet 
} snp_hdr_t;

//-> packet constants
typedef struct packet {
  snp_hdr_t header;
  char data[MAX_PKT_LEN];
} snp_pkt_t;

typedef struct sendpktargument {
  int nextNodeID;    //node ID of the next hop
  snp_pkt_t pkt;         //the packet to be sent
} sendpkt_arg_t;

int overlay_recvpkt(snp_pkt_t* pkt, int overlay_conn);
int overlay_recvpkt(snp_pkt_t* pkt, int overlay_conn);
in_addr_t topology_getIPfromname(char* hostname);
int topology_getNodeIDfromip(struct in_addr* addr);

/**************************************************************/
//declare global variables
/**************************************************************/
int overlay_conn; 		//connection to the overlay


/**************************************************************/
//implementation network layer functions
/**************************************************************/


//this function is used to for the SNP process to connect to the local ON process on port OVERLAY_PORT
//connection descriptor is returned if success, otherwise return -1
int connectToOverlay() {
	int overlaysd; 
	char hostname[50];
	struct sockaddr_in overlay_addr;

	//Find local overlay host
	if (gethostname(hostname, 50) < 0){
		return -1;
	}

	//Set up address connecting to connect to
	memset(&overlay_addr, 0, sizeof(overlay_addr));
	overlay_addr.sin_port = htons(OVERLAY_PORT);
	overlay_addr.sin_addr.s_addr = topology_getIPfromname(hostname);
	overlay_addr.sin_family = AF_INET;

	overlaysd = socket(AF_INET, SOCK_STREAM, 0);
	if (overlaysd < 0){
		printf("err: socket creation failed\n");
		return -1;
	}
	if (connect(overlaysd, (struct sockaddr*)&overlay_addr, sizeof(overlay_addr))<0){
		printf("err: connect failed\n");
		return -1;
	}

	printf("Connected to overlay network\n");
	return overlaysd; 
}

//This thread sends out route update packets every ROUTEUPDATE_INTERVAL time
//In this lab this thread only broadcasts empty route update packets to all the neighbors,
// broadcasting is done by set the dest_nodeID in packet header as BROADCAST_NODEID
void* routeupdate_daemon(void* arg) {
	snp_pkt_t pkt;
	int myID = topology_getMyNodeID();
	pkt.header.src_nodeID = myID; 

	while (overlay_sendpkt(BROADCAST_NODEID, &pkt, overlay_conn) > 0){
		printf("Sent route update packet to overlay\n");
		sleep(ROUTEUPDATE_INTERVAL);
	}
	close(overlay_conn);
	overlay_conn = -100;
	pthread_exit(NULL);
}

//this thread handles incoming packets from the ON process
//It receives packets from the ON process by calling overlay_recvpkt()
//In this lab, after receiving a packet, this thread just outputs the packet received information without handling the packet 
void* pkthandler(void* arg) {
	snp_pkt_t pkt;

	while(overlay_recvpkt(&pkt,overlay_conn)>0) {
		printf("Routing: received a packet from neighbor %d\n",pkt.header.src_nodeID);
	}
	close(overlay_conn);
	overlay_conn = -100;
	pthread_exit(NULL);
}

//this function stops the SNP process 
//it closes all the connections and frees all the dynamically allocated memory
//it is called when the SNP process receives a signal SIGINT
void network_stop() {
	close(overlay_conn);
	overlay_conn = -1;
	printf("Received SIGINT\n");
	exit(0);
}


int main(int argc, char *argv[]) {
	printf("network layer is starting, pls wait...\n");

	//initialize global variables
	overlay_conn = -1;

	//register a signal handler which will kill the process
	signal(SIGINT, network_stop);

	//connect to overlay
	overlay_conn = connectToOverlay();
	if(overlay_conn<0) {
		printf("can't connect to ON process\n");
		exit(1);		
	}
	
	//start a thread that handles incoming packets from overlay
	pthread_t pkt_handler_thread; 
	pthread_create(&pkt_handler_thread,NULL,pkthandler,(void*)0);

	//start a route update thread 
	pthread_t routeupdate_thread;
	pthread_create(&routeupdate_thread,NULL,routeupdate_daemon,(void*)0);	

	printf("network layer is started...\n");

	//sleep forever
	while(1) {
		sleep(30);
		if (overlay_conn == -100){
			printf("Overlay connection failed\n");
			exit(0);
		}
	}
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
