
	/*
	* tunneld.c
	*
	*  Created on: Sep 17, 2016
	*      Author: prashantravi
	*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


#define MAX_BUF 1000

struct sockaddr_storage their_addr;
socklen_t addr_len;


typedef int bool;
	#define true 1
	#define false 0


ssize_t sendsocket(int in_fd, char * hostname, char * hostUDPport);
int create_socket_to_listen_on(char * udpPort);
void startTunnelServer(char* myUDPport);
void registration_proc(int sockfd);
int main(int argc, char** argv)
{
	char* udpPort;

	if(argc != 2)
	{
		fprintf(stderr, "usage: %s vpn_port_number\n\n", argv[0]);
		exit(1);
	}

	udpPort = argv[1];

	startTunnelServer(udpPort);
	return 0;
}
	// Get the address if ipv4 or ipv6 although we only pertain with ipv4
void *get_in_addr(struct sockaddr *sa) {
	return sa->sa_family == AF_INET
	? (void *) &(((struct sockaddr_in*)sa)->sin_addr)
	: (void *) &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int get_in_port(struct sockaddr * sa)
{
	return (int)(((struct sockaddr_in*)sa)->sin_port);
} 

struct InfoPacket
{
	char server_ip[MAX_BUF];
	char server_port[MAX_BUF];
} packet;

	// Start the traffic udp server
void startTunnelServer(char* myUDPport)
{	
	int sockfd = create_socket_to_listen_on(myUDPport);
	registration_proc(sockfd);
	exit(EXIT_SUCCESS);
}


void registration_proc(int sockfd)
{
	char s[INET6_ADDRSTRLEN];
	while (1) {

		int numbytes;
		struct sockaddr_storage new_addr;
		socklen_t new_addr_len;
			// blocking call to receive the traffic sent by traffic_snd.
		bzero(&packet, sizeof packet);

		if ((numbytes = recvfrom(sockfd, &packet, sizeof packet, 0,(struct sockaddr *)&their_addr, &addr_len)) == -1) {
			perror("recvfrom");
			return;
		}
		printf("Received server host %s\n", packet.server_ip);
		printf("Received server port %s\n", packet.server_port);

		printf("Receiver: got packet from %s :%d \n",inet_ntop(their_addr.ss_family,get_in_addr((struct sockaddr *)&their_addr),s, sizeof s), (int) ntohs(get_in_port((struct sockaddr *)&their_addr)));
		

		struct sockaddr_in myaddr;
		if(!fork())
		{	
			int local_listen_sock = create_socket_to_listen_on(NULL);
			sendsocket(local_listen_sock, packet.server_ip, packet.server_port);
			close(local_listen_sock);
		}
	}
	close(sockfd);
	exit(EXIT_SUCCESS);
}


ssize_t sendsocket(int in_fd, char * hostname, char * hostUDPport)
{
	off_t orig;
	char buf[BUFSIZ];
	size_t toRead, numRead, numSent, totSent;

	int sockfd_r;
	struct addrinfo hints, *servinfo;
	int rv;
	int numbytes;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;


	printf(" Want to send to %s %s \n "  , hostname, hostUDPport);
	if ((rv = getaddrinfo(hostname, hostUDPport, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return -1;
	}

	// Setup socket. 
	struct addrinfo *availableServerSockets = servinfo;
	bool connectionSuccessful = false;
	while(availableServerSockets != NULL)
	{
		bool error = false;

		if ((sockfd_r = socket(availableServerSockets->ai_family, availableServerSockets->ai_socktype,availableServerSockets->ai_protocol)) == -1) {//If it fails...
			error = true;
		}
		if(error)
			availableServerSockets = availableServerSockets->ai_next;
		else
		{
			connectionSuccessful = true;
			break;
		}
	}

	if(!connectionSuccessful)
	{
		// Connection was not successful.
		printf("Could not connect to host \n");
		return -1;
	}
	else
	{
		// Successfully connected to server
	}
	totSent = 0;
	while (1) {
		toRead = BUFSIZ;

		numRead = read(in_fd, buf, toRead);

		//printf("%d \n", (int) numRead);
		if (numRead == -1)
			return -1;
		if (numRead == 0)
            break;                      /* EOF */

			numSent = sendto(sockfd_r, buf, numRead , 0, availableServerSockets->ai_addr, availableServerSockets->ai_addrlen);
		if (numSent == -1)
			return -1;
        if (numSent == 0)               /* Should never happen */
		perror("sendfile: write() transferred 0 bytes");
		totSent += numSent;
	}

	close(sockfd_r);
	return totSent;
}


int create_socket_to_listen_on(char *udpPort)
{
	int sockfd_l;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM; // use UDP
	hints.ai_flags = AI_PASSIVE; // use my IP

	if(udpPort == NULL)
	{

    	sockfd_l = socket(AF_INET, SOCK_DGRAM, 0);
    	if (sockfd_l == -1)
    	{
        	printf("socket error\n");
        	exit(1);
    	}
		struct sockaddr_in serv_addr;
		bzero((char *) &serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = INADDR_ANY;
		serv_addr.sin_port = 0;
		if (bind(sockfd_l, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
			if(errno == EADDRINUSE) {
				printf("the port is not available. already to other process\n");
				return;
			} else {
				printf("could not bind to process (%d) %s\n", errno, strerror(errno));
				return;
			}
		}

		socklen_t len = sizeof(serv_addr);
		if (getsockname(sockfd_l, (struct sockaddr *)&serv_addr, &len) == -1) {
			perror("getsockname");
			return;
		}

		printf("port number %d\n", ntohs(serv_addr.sin_port));


		int aInt = 0;
		char str[15];
		sprintf(str, "%d", aInt);

		if ((rv = getaddrinfo(NULL, str, &hints, &servinfo)) != 0) {
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
			return;
		}	

		if (close (sockfd_l) < 0 ) {
			printf("did not close: %s\n", strerror(errno));
			return;
		}
	}
	else
	{
		if ((rv = getaddrinfo(NULL, udpPort, &hints, &servinfo)) != 0) {
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
			return;
		}	
		
	}

		// loop through all the resultant server socket and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd_l = socket(p->ai_family, p->ai_socktype,
			p->ai_protocol)) == -1) {
			perror("receiver: socket");
		continue;
	}

	if (bind(sockfd_l, p->ai_addr, p->ai_addrlen) == -1) {
		close(sockfd_l);
		perror("receiver: bind");
		continue;
	}

	break;
}

if (p == NULL) {
	fprintf(stderr, "receiver: failed to bind socket\n");
	return;
}

addr_len = sizeof their_addr;

printf("Assigned %d \n", ntohs(get_in_port(p->ai_addr)));
freeaddrinfo(servinfo);
uint32_t last_packet_size;
ssize_t recv_size;
return sockfd_l;
}