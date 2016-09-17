/*
 * mypingd.c
 *
 *  Created on: Sep 17, 2016
 *      Author: prashantravi
 */

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

#define MAX_BUF 1000

void make_alphanumeric_string(char *s, const int len);
void sendPingRequest(char* hostname, char* hostUDPport, char* secretKey);
void validateCommandLineArguments(int argc, char ** argv);
void printTimeOfDay(char * message , struct timeval * t);
void printTimeDifference(struct timeval *t1 , struct timeeval *t2);

void make_alphanumeric_string(char *s, const int len) {
	static const char alphanum[] =
			"0123456789"
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			"abcdefghijklmnopqrstuvwxyz";

	for (int i = 0; i < len; ++i) {
		s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
	}
	s[len] = 0;
}

void printTimeDifference(struct timeval *t1 , struct timeeval *t2)
{
	struct timeeval tv1 = *t1;
	struct timeeval tv2 = *t2;
	long milliseconds;
	milliseconds = tv2.tv_usec - tv1.tv_usec / 1000;
	milliseconds += (tv2.tv_sec - tv1.tv_sec) *1000;
	printf("Round trip time was : 3ld\n",milliseconds);
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


void sendPingRequest(char* hostname, char* hostUDPport, char* secretKey)
{
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo(hostname, hostUDPport, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return;
	}

	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("sender: socket");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "sender: failed to create socket\n");
		return;
	}

	if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
	{
		perror("sender: connect");
	}
	freeaddrinfo(servinfo);

	int true = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &true, sizeof(int));

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
	printf("%s\n" , fSendBuffer);

	char recv_response[MAX_BUF];
	struct timeval tv1;
	struct timezone tz1;
	struct timezone tv2;
	struct timezone tz2;
	if (-1 == gettimeofday(&tv1, &tz1)) {
		perror("resettimeofday: gettimeofday");
		exit(-1);
	}
	printTimeOfDay("Sent message", &tv1);
	sendto(sockfd, fSendBuffer, 1000*sizeof(char), 0, (struct sockaddr *)p->ai_addr, sizeof(struct sockaddr));
	recvfrom(sockfd, recv_response, sizeof(recv_response), 0, (struct sockaddr *)p->ai_addr, sizeof(struct sockaddr));
	if (-1 == gettimeofday(&tv2, &tz2)) {
		perror("resettimeofday: gettimeofday");
		exit(-1);
	}
	printTimeDifference(&tv1, &tv2);
	recv_response = ntohl(recv_response);
	printf("sender: got response %s\n", recv_response);
	close(sockfd);
}

int main(int argc, char** argv)
{
	validateCommandLineArguments(argc, argv);
	sendPingRequest(argv[1], argv[2], argv[3]);
	return 0;
}
