
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

void create_send_server_socket_data(int in_fd, char * hostname, char * hostUDPport);
size_t send_socket_data(int in_fd, int out_fd, struct sockaddr * their_addr);
int create_socket_to_listen_on(int *rand_port);
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
	int sockfd ;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM; // use UDP
	hints.ai_flags = AI_PASSIVE; // use my IP


	if ((rv = getaddrinfo(NULL, myUDPport, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return;
	}	

	// loop through all the resultant server socket and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
			p->ai_protocol)) == -1) {
			perror("receiver: socket \n");
		continue;
	}


	if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
		close(sockfd);
		perror("receiver: bind \n");
		continue;
	}

	break;
	}
	if (p == NULL) {
		fprintf(stderr, "receiver: failed to bind socket\n");
		return;
	}

	addr_len = sizeof their_addr;
	freeaddrinfo(servinfo);
	registration_proc(sockfd);
	exit(EXIT_SUCCESS);
}


void registration_proc(int sockfd)
{
	
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

		struct sockaddr_in myaddr;
		if(!fork())
		{	
			int rand_port = 0;
			int local_listen_sock = create_socket_to_listen_on(&rand_port);
			// Let mytunnel know what port has been assigned
			char response[13];
			sprintf(response, "%d", rand_port);
			int n;
			n = sendto(sockfd, response, sizeof response, 0,
					       (struct sockaddr *) &their_addr, addr_len);
			if (n < 0)
			perror("ERROR in sendto\n");
			create_send_server_socket_data(local_listen_sock, packet.server_ip, packet.server_port);
			send_socket_data(local_listen_sock, sockfd, (struct sockaddr*) &their_addr);
			close(sockfd);
			close(local_listen_sock);
		}
	}
	close(sockfd);
	exit(EXIT_SUCCESS);
}



void create_send_server_socket_data(int in_fd, char * hostname, char * hostUDPport)
{
	off_t orig;
	int sockfd_r;
	struct addrinfo hints, *servinfo;
	int rv;
	int numbytes;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;


	if ((rv = getaddrinfo(hostname, hostUDPport, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return ;
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
	return ;
}
else
{
	// Successfully connected to server
}
send_socket_data(in_fd, sockfd_r, availableServerSockets->ai_addr);
close(sockfd_r);
}


size_t send_socket_data(int in_fd, int out_fd, struct sockaddr * their_addr)
{
	char buf[BUFSIZ];
	size_t toRead, numRead, numSent, totSent;
	while (1) {
	toRead = BUFSIZ;

	numRead = read(in_fd, buf, toRead);

	//printf("%d \n", (int) numRead);
	if (numRead == -1)
		return -1;
	if (numRead == 0)
        break;                      /* EOF */

		numSent = sendto(out_fd, buf, numRead , 0, their_addr, sizeof (struct sockaddr));
	if (numSent == -1)
		return -1;
    if (numSent == 0)               /* Should never happen */
	perror("sendfile: write() transferred 0 bytes \n");
	totSent += numSent;
	}
	return totSent;

}


int create_socket_to_listen_on(int *rand_port)
{
	int   sd;
	struct sockaddr_in server;
	struct sockaddr_in foo;
	int len = sizeof(struct sockaddr);
	char buf[512];
	int rc;
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(0);
	sd = socket (AF_INET,SOCK_DGRAM,0);
	bind ( sd, (struct sockaddr *) &server, sizeof(server));
	getsockname(sd, (struct sockaddr *) &foo, &len);
    *rand_port = ntohs(foo.sin_port);
	return sd;
}