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

typedef int bool;
#define true 1
#define false 0

#define MAX_BUF 1000
#define EXPECTED_RESPONSE "terve"
#define TIME_OUT 550000
#define INTERVAL 50000

volatile bool received = false;
pid_t current_pid = -1 ;

int sendMyTunnelRequest(char* hostname, char* hostUDPport);
void validateCommandLineArguments(int argc, char ** argv);
long getTimeDifference(struct timeval *t1 , struct timeval *t2);

void validateCommandLineArguments(int argc, char ** argv)
{
	if(argc != 5)
	{
		printf("\nERROR! (expected 4 , got %d) usage: %s VPN_IP VPN_PORT SERVER_IP SERVER_PORT\n\n", argc, argv[0]);
		exit(1);
	}
}
/*
	Send the traffic using the designated arguments. 


*/


int sendMyTunnelRequest(char* hostname, char* hostUDPport)
{

	int sockfd;
	struct addrinfo hints, *servinfo;
	int rv;
	int numbytes;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo(hostname, hostUDPport, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 2;
	}
	char payload[] = {'R'};

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
	char recv_response[6];
	struct sockaddr_in from;
	socklen_t addr_len = sizeof from;
	socklen_t sin_size = sizeof(struct sockaddr);

	int packets_sent = 0;
	int packet_counter = 0;

	// Send the packetCount number of packets of designated size
	if((packets_sent = sendto(sockfd, payload, sizeof payload , 0, availableServerSockets->ai_addr, availableServerSockets->ai_addrlen)) == -1)
	{	
		perror("sendto: failed\n");
		exit(EXIT_FAILURE);
	}

	// Account for the bytes in UDP header 
	return EXIT_SUCCESS;
}
int main(int argc, char** argv)
{
	// valdidate that the command line arguments are correct.
	validateCommandLineArguments(argc, argv);
	current_pid = getpid();
	return sendMyTunnelRequest(argv[1], argv[2]);
}