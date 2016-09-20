/*


 * mypingd.c
 *
 *  Created on: Sep 17, 2016
 *      Author: prashantravi
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
volatile sig_atomic_t got_interrupt = 0;
pid_t current_pid = -1 ;

void make_alphanumeric_string(char *s, const int len);
int sendPingRequest(char* hostname, char* hostUDPport, char* secretKey);
void validateCommandLineArguments(int argc, char ** argv);
void printTimeDifference(struct timeval *t1 , struct timeval *t2);
void handle_signal_alarm(int sig);

void handle_signal_alarm(int sig)
{
	int saved_errno = errno;
	if (sig != SIGALRM) {
		perror("Caught wrong signal\n");
	}
	if(received == false && got_interrupt++ == 40){
		printf( "no response from ping server\n" );
		kill(current_pid,SIGKILL);
	}
	errno = saved_errno;
}


void make_alphanumeric_string(char *s, const int len) {
	static const char alphanum[] =
			"0123456789"
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			"abcdefghijklmnopqrstuvwxyz";

	int i;
	for ( i = 0; i < len; ++i) {
		s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
	}
	s[len] = 0;
}

void printTimeDifference(struct timeval *t1 , struct timeval *t2)
{
	struct timeval tv1 = *t1;
	struct timeval tv2 = *t2;
	long milliseconds;
	milliseconds = (tv2.tv_usec - tv1.tv_usec) / 1000;
	milliseconds += (tv2.tv_sec - tv1.tv_sec) *1000;
	printf("Round trip time was : %3ld ms\n",milliseconds);
}

void validateCommandLineArguments(int argc, char ** argv)
{
	// Write this validation properly TODO
	if(argc != 4)
	{
		printf("\nERROR! usage: %s receiver_hostname receiver_port secret_key\n\n", argv[0]);
		exit(1);
	}
}


int sendPingRequest(char* hostname, char* hostUDPport, char* secretKey)
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
		printf("Could not connect to host \n");
		return 2;
	}
	else
	{
		// Successfully connected to server
	}
	int tr = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tr, sizeof(int));

	// Construct message with format $secretKey$pad to be 1000 bytes
	char str[MAX_BUF];
	//Get length of secret key.
	int dlen = (strlen(secretKey)) * sizeof(char);
	sprintf(str, "$%s$", secretKey);
	dlen = dlen + 2*sizeof(char);
	//Find how many alphanumeric characters we need to pad
	int dpad = MAX_BUF - dlen;
	//Our final sending buffer
	char fSendBuffer[MAX_BUF] = {0};
	memcpy(fSendBuffer, str , dlen);
	make_alphanumeric_string(fSendBuffer+dlen, dpad );

	char recv_response[6];
	struct timeval tv1;
	struct timezone tz1;
	struct timeval tv2;
	struct timezone tz2;
	struct sockaddr_in from;
	socklen_t addr_len = sizeof from;
	socklen_t sin_size = sizeof(struct sockaddr);
	signal(SIGALRM, handle_signal_alarm);
	ualarm(TIME_OUT,INTERVAL);
	if (-1 == gettimeofday(&tv1, &tz1)) {
		perror("resettimeofday: gettimeofday");
		exit(-1);
	}
	if(sendto(sockfd, fSendBuffer, strlen(fSendBuffer), 0, availableServerSockets->ai_addr, availableServerSockets->ai_addrlen) == -1)
	{
		perror("sendto: failed\n");
	}
	socklen_t addrlen = sizeof(from); /* must be initialized */
	recvfrom(sockfd, recv_response, sizeof(recv_response), 0, (struct sockaddr *)&from, &addrlen);
	received = true;
	if (-1 == gettimeofday(&tv2, &tz2)) {
		perror("resettimeofday: gettimeofday");
		exit(-1);
	}
	recv_response[5] = '\0';
	struct hostent *hostp; /* server host info */
	hostp = gethostbyaddr((const char *)&from.sin_addr.s_addr,
			sizeof(from.sin_addr.s_addr), AF_INET);
	if (hostp == NULL)
		perror("ERROR on gethostbyaddr");
	char *hostaddrp; /* dotted decimal server host addr string */
	hostaddrp = inet_ntoa(from.sin_addr);
	if (hostaddrp == NULL)
		perror("ERROR on inet_ntoa\n");
	if(strcmp(EXPECTED_RESPONSE, recv_response ) == 0)
	{
		printTimeDifference(&tv1, &tv2);
	}
	else
		printf("Packet received was corrupt : %s\n", recv_response);
	printf("Client received datagram from %s (%s)\n",hostp->h_name, hostaddrp);
	close(sockfd);
	return EXIT_SUCCESS;
}

int main(int argc, char** argv)
{
	validateCommandLineArguments(argc, argv);
	current_pid = getpid();
	return sendPingRequest(argv[1], argv[2], argv[3]);
}
