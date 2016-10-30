
/*
* tunneld.c
* Created on: Sep 17, 2016
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
#include <fcntl.h>

#define MAX_BUF 1000

typedef int bool;
#define true 1
#define false 0

int create_send_server_socket_data(int in_fd, char * hostname, char * hostUDPport);
size_t send_socket_data(int in_fd, struct sockaddr * server_addr);
int create_socket_to_listen_on(int *rand_port);
void startTunnelServer(char* myUDPport);
void registration_proc(int sockfd);


int main(int argc, char** argv)
{
	char* udpPort;

	if (argc != 2)
	{
		fprintf(stderr, "usage: %s vpn_port_number\n\n", argv[0]);
		exit(1);
	}

	udpPort = argv[1];

	startTunnelServer(udpPort);
	return 0;
}
// Get the address if ipv4 or ipv6 although we only pertain with ipv4
char *get_in_addr(struct sockaddr *sa) {
	return (char *) inet_ntoa((((struct sockaddr_in*)sa)->sin_addr));
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
	for (p = servinfo; p != NULL; p = p->ai_next) {
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

	
	freeaddrinfo(servinfo);
	registration_proc(sockfd);
	exit(EXIT_SUCCESS);
}


void registration_proc(int sockfd)
{

	struct sockaddr_storage their_addr;
	socklen_t addr_len ;
	addr_len = sizeof their_addr;
	while (1) {
		int numbytes;
		struct sockaddr_storage new_addr;
		socklen_t new_addr_len;
		// blocking call to receive the traffic sent by traffic_snd.
		bzero(&packet, sizeof packet);
		if ((numbytes = recvfrom(sockfd, &packet, sizeof packet, 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
			perror("recvfrom");
			return;
		}
		if (!fork())
		{
			int rand_port = 0;
			int local_listen_sock = create_socket_to_listen_on(&rand_port);
			// Let mytunnel know what port has been assigned
			char response[13];
			sprintf(response, "%d", rand_port);
			int n;
			n = sendto(local_listen_sock, response, sizeof response, 0,
			           (struct sockaddr *) &their_addr, addr_len);
			if (n < 0)
				perror("ERROR in sendto\n");
			//printf("Now send client data to server \n");

			struct	sockaddr_in server;
			struct  hostent *hp, *gethostbyname();

			server.sin_family = AF_INET;
			hp = gethostbyname(packet.server_ip);
			bcopy ( hp->h_addr, &(server.sin_addr.s_addr), hp->h_length);
			server.sin_port = htons(atoi(packet.server_port));

			//printf("host name  %s | host port %d \n", packet.server_ip, ntohs(server.sin_port));
			struct sockaddr * client_addr = NULL;
			send_socket_data(local_listen_sock,(struct sockaddr*) &server);
			close(local_listen_sock);
		}
	}
	close(sockfd);
	exit(EXIT_SUCCESS);
}

size_t send_socket_data(int in_fd, struct sockaddr * server_addr)
{

	char buf[BUFSIZ];
	size_t toRead, numRead, numSent, totSent;
	struct sockaddr recv_sock;
	struct sockaddr test_recv_sock;
	socklen_t addr_len = sizeof (recv_sock);
	struct sockaddr * who_to_send_addr;
	struct sockaddr* client_addr = NULL;
	while (1) {
		toRead = BUFSIZ;
		if(client_addr == NULL)
		{
			numRead = recvfrom(in_fd, buf, toRead, 0, &recv_sock, &addr_len);

			if(numRead == -1) continue;

		//	printf("numread is %d\n", (int) numRead);
			client_addr = &recv_sock;

			char * opp_hostname = get_in_addr((struct sockaddr *)&recv_sock);
			int opp_port =  ntohs(get_in_port((struct sockaddr *)&recv_sock));

			char * opp_hostname_chk = get_in_addr((struct sockaddr *)server_addr);
			int opp_port_chk =  ntohs(get_in_port((struct sockaddr *)server_addr));


			// printf("LHS1 is %s %d \n", opp_hostname, opp_port);
			// printf("RHS1 is %s %d \n", opp_hostname_chk, opp_port_chk);

			if(!strcmp(opp_hostname, opp_hostname_chk) && opp_port == opp_port_chk)
			{
				printf("No client so server can't send to anyone. \n ");
				client_addr = NULL;
				continue;
			}

			who_to_send_addr = server_addr;
		}
		else
		{
			numRead = recvfrom(in_fd, buf, toRead, 0 , &test_recv_sock, &addr_len);
			
			if(numRead == -1) continue;

//			printf("numread is %d\n", (int) numRead);
			char * opp_hostname = get_in_addr((struct sockaddr *)&test_recv_sock);
			int opp_port =  ntohs(get_in_port((struct sockaddr *)&test_recv_sock));

			char * opp_hostname_chk = get_in_addr((struct sockaddr *)server_addr);
			int opp_port_chk =  ntohs(get_in_port((struct sockaddr *)server_addr));


			// printf("LHS2 is %s %d \n", opp_hostname, opp_port);
			// printf("RHS2 is %s %d \n", opp_hostname_chk, opp_port_chk);

			if(!strcmp(opp_hostname, opp_hostname_chk) && opp_port == opp_port_chk)
			{
	//			printf("who to send is client \n");
				who_to_send_addr = client_addr;
			}
			else
			{
				//printf("who to send is server \n");
				who_to_send_addr = server_addr;
			}

		}                 /* EOF */
		numSent = sendto(in_fd, buf, numRead , 0, who_to_send_addr, addr_len);
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
	sd = socket (AF_INET, SOCK_DGRAM, 0);
	bind ( sd, (struct sockaddr *) &server, sizeof(server));
	getsockname(sd, (struct sockaddr *) &foo, &len);
	*rand_port = ntohs(foo.sin_port);
	int sockfd = sd;
	int flags = 0;

	if (-1 == (flags = fcntl(sockfd, F_GETFL, 0)))
		flags = 0;

	if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0)
	{
		perror("Cant' make socket non blocking");
		exit(EXIT_FAILURE);
	}
	return sd;
}