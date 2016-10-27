
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

	void startTunnelServer(char* myUDPport);
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
	// Start the traffic udp server
	void startTunnelServer(char* myUDPport)
	{
		struct timeval tv1;
		struct timeval tv2;

		int sockfd;
		struct addrinfo hints, *servinfo, *p;
		int rv;
		int numbytes;
		struct sockaddr_storage their_addr;
		socklen_t addr_len;
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
			char request[BUFSIZ];

			freeaddrinfo(servinfo);

			uint32_t last_packet_size;
			ssize_t recv_size;

			printf("Tunnel Server Started !\n ");
			int total_num_bytes_recvd = 0;
			while (1) {
				struct sockaddr_storage new_addr;
				socklen_t new_addr_len;

				// blocking call to receive the traffic sent by traffic_snd.
				bzero((char *) request, sizeof(request));

				if ((numbytes = recvfrom(sockfd, request, sizeof(request), 0,
				(struct sockaddr *)&their_addr, &addr_len)) == -1) {
					perror("recvfrom");
					return;
				}

				printf(" Received %s \n", request);
			}
			close(sockfd);
			exit(EXIT_SUCCESS);
		}
