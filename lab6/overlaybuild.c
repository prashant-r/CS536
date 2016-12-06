/*
 * mypingd.c
 *
 *  Created on: Sep 17, 2016
 *      Author: prashantravi
 *
 */


#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>

typedef int bool;
#define true 1
#define false 0

#define MAX_BUF 1000
#define EXPECTED_RESPONSE "terve"
#define TIME_OUT 550000
#define INTERVAL 50000

volatile bool received = false;
pid_t current_pid = -1 ;


int sendMyTunnelRequest(int argc, char ** argv);
void validateCommandLineArguments(int argc, char ** argv);
long getTimeDifference(struct timeval *t1 , struct timeval *t2);

void validateCommandLineArguments(int argc, char ** argv)
{
	if(argc <= 5)
	{
		printf("\nERROR! (expected 5 , got %d) usage: %s dst-IP dst-port routerk-IP ... router2-IP router1-IP overlay-port build-port \n\n", argc, argv[0]);
		exit(1);
	}
}
/*
	Send the traffic using the designated arguments. 
*/

bool isnumber(const char*s);


struct InfoPacket
{
   	char server_host[MAX_BUF];
    char server_port[MAX_BUF];
} packet;

bool isnumber(const char*s) {
   char* e = NULL;
   (void) strtol(s, &e, 0);
   return e != NULL && *e == (char)0;
}
int sendMyTunnelRequest(int argc, char ** argv)
{

	int sockfd;
	struct addrinfo hints, *servinfo;
	int rv;
	int numbytes;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	char *dst_host = argv[1];
	char *dst_port = argv[2];

	char str[1000];
	strcpy(str, "$");
	strcat(str, dst_host);
	strcat(str, "$");
	strcat(str, dst_port);
	strcat(str, "$");

	int num_forwards = argc -2 -2;
	char * forwards[num_forwards];
	int a;
	for(a = 3; a < argc-2; a++)
	{
		forwards[a] = argv[a];
		//printf(" Forwards %d | %s \n", a, forwards[a]);
		strcat(str, forwards[a]);
		strcat(str, "$");
	}
	char * hostname;
	char * port;
	if(num_forwards == 0)
	{
		hostname = dst_host;
		port = dst_port;
	}
	else
	{
		hostname = argv[argc-3];
		port = argv[argc-2];
	}
	char * overlay_port  = argv[argc-2];
	char * build_port = argv[argc-1];

	//printf("overlay_port is %s \n", overlay_port);
	//printf("build port is %s \n", build_port);
	// create the payload
	//printf("Payload is %s \n", str);
	//printf("Hostname is %s \n", hostname);
	//printf("Port is %s\n", port);
	
	if ((rv = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 2;
	}

	// Setup socket. 
	struct addrinfo *availableServerSockets = servinfo;
	bool connectionSuccessful = false;
	while(availableServerSockets != NULL)
	{
		bool error = false;

		if ((sockfd = socket(availableServerSockets->ai_family, availableServerSockets->ai_socktype,availableServerSockets->ai_protocol)) == -1) {//If it fails...
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
		return 2;
	}
	else
	{
		// Successfully connected to server
	}
	int tr = 1;
	struct sockaddr_in from;
	socklen_t addr_len = sizeof from;
	socklen_t sin_size = sizeof(struct sockaddr);
	socklen_t addrlen = sizeof(from); /* must be initialized */


	char recv_response[13];

	int packets_sent = 0;
	int packet_counter = 0;

	//printf("rserver_host %s %lu , rserver_port %s %lu \n", packet.server_host,sizeof(packet.server_host), packet.server_port, sizeof(packet.server_port));

	// Send the packetCount number of packets of designated size
	if((packets_sent = sendto(sockfd, str , strlen(str) , 0, availableServerSockets->ai_addr, availableServerSockets->ai_addrlen)) == -1)
	{	
		perror("sendto: failed\n");
		exit(EXIT_FAILURE);
	}

	recvfrom(sockfd, recv_response, sizeof(recv_response), 0, (struct sockaddr *)&from, &addrlen);

	struct hostent *hostp; /* server host info */
	hostp = gethostbyaddr((const char *)&from.sin_addr.s_addr,
			sizeof(from.sin_addr.s_addr), AF_INET);
	if (hostp == NULL)
		perror("ERROR on gethostbyaddr");
	char *hostaddrp; /* dotted decimal server host addr string */
	hostaddrp = inet_ntoa(from.sin_addr);


	if(isnumber(recv_response ))
	{
		if(atoi(recv_response) == -1)
		{
			printf("\nFailure! Probable cause: invalid server host provided. \n");
		}
		else
			printf("\nSuccess! Make requests on port : %s\n", recv_response);
	}
	else
	{
		if(recv_response[0] == 'A')
		{
			printf("Route NOT established after 30 second wait. \n");
		}
		else
		{
			printf("Packet received was corrupt : %s\n", recv_response);
		}
	}
	if (hostaddrp == NULL)
		perror("ERROR on inet_ntoa\n");
	return EXIT_SUCCESS;
}
int main(int argc, char** argv)
{
	// valdidate that the command line arguments are correct.
	validateCommandLineArguments(argc, argv);
	current_pid = getpid();
	return sendMyTunnelRequest(argc, argv);
}