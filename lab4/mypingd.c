/*

 * myping.c
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

void startServer(char* myUDPport, char* secretKey);

int main(int argc, char** argv)
{
	char* udpPort;

	if(argc != 3)
	{
		fprintf(stderr, "usage: %s UDP_port secret_key\n\n", argv[0]);
		exit(1);
	}

	udpPort = argv[1];

	startServer(udpPort, argv[2]);
	return 0;
}

void *get_in_addr(struct sockaddr *sa) {
	return sa->sa_family == AF_INET
			? (void *) &(((struct sockaddr_in*)sa)->sin_addr)
					: (void *) &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void startServer(char* myUDPport, char* secretKey)
{
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	struct sockaddr_storage their_addr;
	socklen_t addr_len;
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, myUDPport, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("receiver: socket");
			continue;
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
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
	char request[1000];

	freeaddrinfo(servinfo);

	char * response = "terve";

	uint32_t last_packet_size;
	ssize_t recv_size;


	int request_number = 1 ;
	while (1) {
		struct sockaddr_storage new_addr;
		socklen_t new_addr_len;
		bzero((char *) request, sizeof(request));
		printf("receiver: waiting for client connection \n");
		if ((numbytes = recvfrom(sockfd, request, sizeof(request), 0,
			        (struct sockaddr *)&their_addr, &addr_len)) == -1) {
			        perror("recvfrom");
			        return;
		}
		if(request_number == 4)
		{
			request_number = 1;
			continue;
		}
		request_number ++;
		if (numbytes == 1000) {

			char *saveptr;
			char *secretKeyDecode, *pad;
			secretKeyDecode = strtok_r(request, "$", &saveptr);
			pad = strtok_r(NULL, "$", &saveptr);
			//printf("request %s \n", request);
			//printf("secretKeyDecode%s\n", secretKeyDecode);
			//printf("secretKey %s\n", secretKey);
			if(strcmp(secretKeyDecode, secretKey) == 0)
			{
				int n;
				n = sendto(sockfd, response, strlen(response), 0,
					       (struct sockaddr *) &their_addr, addr_len);
				if (n < 0)
				   perror("ERROR in sendto\n");
			}
			else
			{
				perror("receiver: secretKey doesn't match ");
				continue;
			}
		}
		else{
			perror("receiver: recv not 1000 bytes");
			continue;
		}
	}

	close(sockfd);
}