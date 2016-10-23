
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
long getTimeDifference(struct timeval *t1 , struct timeval *t2);
int main(int argc, char** argv)
{
	char* udpPort;

	if(argc != 2)
	{
		fprintf(stderr, "usage: %s UDP_port\n\n", argv[0]);
		exit(1);
	}

	udpPort = argv[1];

	startServer(udpPort, argv[2]);
	return 0;
}
// Get the address if ipv4 or ipv6 although we only pertain with ipv4
void *get_in_addr(struct sockaddr *sa) {
	return sa->sa_family == AF_INET
			? (void *) &(((struct sockaddr_in*)sa)->sin_addr)
					: (void *) &(((struct sockaddr_in6*)sa)->sin6_addr);
}
// Helper function to compute the time difference between two timevals 
// to the order of milliseonds.
long getTimeDifference(struct timeval *t1 , struct timeval *t2)
{
	struct timeval tv1 = *t1;
	struct timeval tv2 = *t2;
	long milliseconds;
	milliseconds = (tv2.tv_usec - tv1.tv_usec) / 1000;
	milliseconds += (tv2.tv_sec - tv1.tv_sec) *1000;
	return milliseconds;
}
// Start the traffic udp server
void startServer(char* myUDPport, char* secretKey)
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

	int request_number = 1 ;
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
		// If we are at first request then star the timer.
		if(request_number == 1)
		{
			if (-1 == gettimeofday(&tv1, NULL)) {
				perror("resettimeofday: gettimeofday");
				exit(-1);
			}
		}
		// If we have received the 3 byte packet then stop the timer.
		if (numbytes == 3) {
			if (-1 == gettimeofday(&tv2, NULL)) {
				perror("resettimeofday: gettimeofday");
				exit(-1);
			}
			break;
		}
		else
		{
			// increment the request number with each incoming packet.
			request_number ++;
			// Account for the UDP bytes in payload for computing application bitrate in bps.
			total_num_bytes_recvd += (numbytes + 46);
		}
	}
	// Convert bytes to bits.
	long long int total_num_bits = 8*total_num_bytes_recvd;

	long ms = getTimeDifference(&tv1,&tv2);
	double time_duration = ms/1000.0;
	printf("Completion time: %lf seconds\n", time_duration);
	printf("Application bitrate: %lf pps\n", (double) request_number/time_duration);
    printf("Application bitrate: %lf bps\n", (double)(total_num_bits)/time_duration);
	close(sockfd);
	exit(EXIT_SUCCESS);
}