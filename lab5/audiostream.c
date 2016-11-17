/*
* audiostream.c
* Created on: Nov 15, 2016
*      Author: prashantravi
*/
#define _GNU_SOURCE
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
#include <signal.h>
#include <stdbool.h>

#define BUFSIZE 8192
#define PORTBUFSIZE 199
#define MAX_BUF 1000
#define MAX_WAITING_CLIENTS 10

FILE *file;
FILE *logFile;
size_t nread;
struct sockaddr_in * client_to_transact_with;
int sock_fd;
int mode;
size_t pySz;
size_t pktSpcing;
struct timeval tv1;
struct timeval tv2;

int create_send_server_socket_data(int in_fd, char * hostname, char * hostUDPport);
size_t send_socket_data(int in_fd, char * message, struct sockaddr * server_addr);
int  create_socket_to_listen_on(char *rand_port);
void startTunnelServer(char* myTCPport, char* logfileS, char *udpPort, char* payloadSize, char* packetSpacing, char* mode);
void registration_proc(int sockfd, char * logfileS, char* udpPort, char* payloadSize, char* packetSpacing, char* mode);
bool isValidIpAddress(char *ipAddress);
void handle_sigchld(int sig);
void handle_packet_transfer(FILE * logFile);
void handle_sigpoll(int sig);
void handle_child_request(FILE * logfileS, char* udpPort, char* payloadSize, char* packetSpacing, char* mode);
char * trimwhitespace(char *str);
long getTimeDifference(struct timeval *t1 , struct timeval *t2);
size_t recv_socket_data(int in_fd, char * buffer);
void do_sleep(useconds_t time);
typedef enum {BUSY, READY} STATES;

volatile STATES state = READY;

void handle_sigchld(int sig) {
	int saved_errno = errno;
	while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}
		errno = saved_errno;
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

void handle_sigalrm(int signal) {
    if (signal != SIGALRM) {
        fprintf(stderr, "Caught wrong signal: %d\n", signal);
    }
    //printf("Signal handler called \n");
	char buf[pySz];
	if (file) {
		if((nread = fread(buf, 1, sizeof buf, file)) > 0)
		{
			send_socket_data(sock_fd, buf, client_to_transact_with);
        	//printf("%s\n", buf);
			if (ferror(file)) {
        		/* deal with error */
				file = NULL;
			}
			struct timeval tz2; 
			if (-1 == gettimeofday(&tz2, NULL)) {
				perror("resettimeofday: gettimeofday");
				exit(-1);
			}
			long ms = getTimeDifference(&tv1,&tz2);
			double time_duration = ms/1000.0;
			fprintf(logFile, "Time: %lf sec | Packet Spacing: %d  \n", time_duration, pktSpcing);
		}
		else
		{
			printf("Done \n");

			size_t numSent;
			struct sockaddr recv_sock;
			socklen_t addr_len = sizeof(recv_sock);
			struct sockaddr * who_to_send_addr = client_to_transact_with;
			int packets_sent = 0;
			char lastThreeBytes[] = {'G', 'O', 'D'};
			int g =0;
			// Send the last 3 3-byte packets without stopping.
			for (g = 0; g< 3; g++)
			{

				if((packets_sent = sendto(sock_fd, lastThreeBytes, 3 , 0, who_to_send_addr, addr_len))== -1)
				{
					perror("sendto: failed\n");
					exit(EXIT_FAILURE);
				}
			}

			// TODO : reached end of file.
			if(file)
				fclose(file);
			file = NULL;
			state = READY;
		}
	}
}

void handle_sigpoll(int sig){
	int saved_errno = errno;

	char buffer[50];
	// parse the incoming message
	recv_socket_data(sock_fd, buffer);

	char * pch;
	char * client_command_r = strtok (buffer," ");
	char * client_q_star_r = strtok (NULL, " ");
	char * client_q_r = strtok(NULL, " ");
	char * client_tau_r = strtok( NULL, " ");
	
	//printf("Congestion control called %s %s %s %s %s \n", client_command_r, client_q_star_r, client_q_r, client_tau_r);

	if(*client_command_r == 'Q')
	{

		int qt = atoi(client_q_r);
		int q_star = atoi(client_q_star_r);
		int tau = atoi(client_tau_r);
		int a = 3;
		double delta = 0.30;
		double epsilon = 0.0139;
		double beta = 0.5;
		printf("Bef pcktSpcing %d \n", pktSpcing);
		if(mode == 0)
		{
			//if Q(t) = Q∗ then λ(t + 1) ← λ(t)
			if(qt == q_star)
			{
				pktSpcing = pktSpcing *1;
			}
			//if Q(t) < Q∗ then λ(t + 1) ← λ(t) + a
			else if( qt < q_star)
			{
				pktSpcing = pktSpcing + a;
			}
			//if Q(t) > Q∗ then λ(t + 1) ← λ(t) − a
			else
			{
				pktSpcing = pktSpcing - a;
			}
		}
		else if(mode == 1)
		{
			//if Q(t) = Q∗ then λ(t + 1) ← λ(t)
			if(qt == q_star)
			{
				pktSpcing = pktSpcing *1;
			}
			//if Q(t) < Q∗ then λ(t + 1) ← λ(t) + a
			else if(qt < q_star)
			{
				pktSpcing = pktSpcing + a;
			}
			//if Q(t) > Q∗ then λ(t + 1) ← δ × λ(t)
			else
			{
				pktSpcing = delta * pktSpcing; 
			}
		}
		else if(mode == 2)
		{
			//λ(t + 1) ← λ(t) + ε(Q∗ − Q(t))
			pktSpcing = pktSpcing + epsilon * (q_star - qt);
		}
		else if(mode == 3)
		{
			//λ(t + 1) ← λ(t) + ε(Q∗ − Q(t)) − β(λ(t) − γ)
			pktSpcing = pktSpcing + (epsilon*(q_star - qt)) - beta * (pktSpcing - tau);
		}

		printf("Aft pcktSpcing %d \n", pktSpcing);
		if(pktSpcing < 0 ) pktSpcing =1;
		if(pktSpcing >= 1000) pktSpcing = 999;
	}
	errno = saved_errno;
}

size_t send_socket_data(int in_fd, char * message, struct sockaddr * server_addr)
{
	size_t numSent;
	struct sockaddr recv_sock;
	socklen_t addr_len = sizeof(recv_sock);
	struct sockaddr * who_to_send_addr = server_addr;
	numSent = sendto(in_fd, message, strlen(message) , 0, who_to_send_addr, addr_len);
	return numSent;
}


int main(int argc, char** argv)
{
	printf("SERVER ON. \n");
	char* tcpPort;
	char* udpPort;
	char* payloadSize;
	char* packetSpacing;
	char* logfileS;
	if (argc != 7)
	{
		fprintf(stderr, "usage: %s tcp-port udp-port payload-size packet-spacing mode logfile-s\n\n", argv[0]);
		exit(1);
	}
	tcpPort = argv[1];
	udpPort = argv[2];
	payloadSize = argv[3];
	pySz = atoi(payloadSize);
	packetSpacing = argv[4];
	pktSpcing = atoi(packetSpacing);
	mode = atoi(argv[5]);
	logfileS = argv[6];
	startTunnelServer(tcpPort, logfileS, udpPort, payloadSize, packetSpacing, mode);
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

char * dataPacket;
// Start the traffic udp server
void startTunnelServer(char* myTCPport, char* logfileS, char* udpPort, char* payloadSize, char* packetSpacing, char* mode)
{
	int sockfd ;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];
	dataPacket = (char *)malloc(sizeof(char) * *payloadSize);
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM; // use TCP
	hints.ai_flags = AI_PASSIVE; // use my IP


	if ((rv = getaddrinfo(NULL, myTCPport, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return;
	}

	// loop through all the resultant server socket and bind to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
			p->ai_protocol)) == -1) {
			//perror("receiver: socket \n");
			continue;
	}


	if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
		close(sockfd);
			//perror("receiver: bind \n");
		continue;
	}

	break;
}
if (p == NULL) {
	fprintf(stderr, "receiver: failed to bind socket\n");
	return;
}	
freeaddrinfo(servinfo);
registration_proc(sockfd, logfileS, udpPort, payloadSize, packetSpacing, mode);
exit(EXIT_SUCCESS);
}

char *trimwhitespace(char *str)
{
	char *end;

  // Trim leading space
	while(isspace((unsigned char)*str)) str++;

  if(*str == 0)  // All spaces?
  	return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator
  *(end+1) = 0;

  return str;
}


void registration_proc(int sockfd, char * logfileS, char* udpPort, char* payloadSize, char* packetSpacing, char* mode)
{
 	int childfd; /* child socket */
  	int clientlen; /* byte size of client's address */
  	struct sockaddr_in clientaddr; /* client addr */
  	struct hostent *hostp; /* client host info */
  	char buf[BUFSIZE]; /* message buffer */
  	char portbuf[PORTBUFSIZE]; /* port message buffer*/
  	char *hostaddrp; /* dotted decimal host addr string */
  	int n; /* message byte size */

	struct sockaddr_storage their_addr;
	socklen_t addr_len ;
	addr_len = sizeof their_addr;

	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));

	sa.sa_sigaction = (void (*)(int))&handle_sigpoll;
	sa.sa_flags = SA_SIGINFO;

	if (!((sigaction(SIGIO, &sa, NULL ) == 0) && (sigaction(SIGPOLL, &sa, NULL) == 0)))
	{
		perror("Can't create signal action.");
		exit(EXIT_FAILURE);
	}

	// open / create the log file
    /* open the file */
	logFile = fopen(logfileS, "wb");
	if (logFile == NULL) {
		printf("I couldn't open logfile_s for appending.\n");
		exit(0);
	}
	/* 
   	 * listen: make this socket ready to accept connection requests 
   	 */
  	if (listen(sockfd, MAX_WAITING_CLIENTS) < 0) /* allow 5 requests to queue up */ 
	error("ERROR on listen");

  	/* 
   	 * main loop: wait for a connection request, echo input line, 
   	 * then close connection.
  	 */
	clientlen = sizeof(clientaddr);
	while (1) {

		signal(SIGCHLD, handle_sigchld); 
    	/* 
     	 * accept: wait for a connection request 
     	 */
		childfd = accept(sockfd, (struct sockaddr *) &clientaddr, &clientlen);
		if (childfd < 0) 
			error("ERROR on accept");

    	/* 
     	* gethostbyaddr: determine who sent the message 
     	*/
		hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, 
			sizeof(clientaddr.sin_addr.s_addr), AF_INET);
		if (hostp == NULL)
			error("ERROR on gethostbyaddr");
		hostaddrp = inet_ntoa(clientaddr.sin_addr);
		if (hostaddrp == NULL)
			error("ERROR on inet_ntoa\n");
		printf("server established connection with %s (%s)\n", hostp->h_name, hostaddrp);

    	/* 
     	* read: read input string from the client
     	*/
		bzero(buf, BUFSIZE);
		n = read(childfd, buf, BUFSIZE);
		if (n < 0) 
			error("ERROR reading from socket");

    	// Split the received buf by whitespace
		char * pch;
		char * client_port_r = strtok (buf," ");
		char * client_path_r = strtok (NULL, " ");

		trimwhitespace(client_port_r);
		trimwhitespace(client_path_r);

		struct	sockaddr_in server;
		server.sin_family = AF_INET;
		struct hostent *hp, *gethostbyname();
		hp = gethostbyname(hostp->h_name);
		bcopy ( hp->h_addr, &(server.sin_addr.s_addr), hp->h_length);
		server.sin_port = htons(atoi(client_port_r));
		client_to_transact_with = &server;

  		// filename
		if( access(client_path_r, F_OK ) != -1 ) {
			memset(portbuf, 0, PORTBUFSIZE);
			char okport[15];
			bzero(okport, 15);
			strcat(okport, "OK ");
			strcat(okport,udpPort);
			strcpy(portbuf, okport);
			n = write(childfd, portbuf, strlen(portbuf));
			char *mode = "r";
			file = fopen(client_path_r, mode);
			if (n < 0) 
				error("ERROR writing to socket");
		} else {
			memset(buf, 0, sizeof(buf));
			strcpy(buf, "KO");
			/* 
     	 	* write: echo the input string back to the client 
     	 	*/
			n = write(childfd, buf, strlen(buf));
			if (n < 0) 
				error("ERROR writing to socket");
			printf("File doesn't exist \n");
			continue;
		}

		int forked = fork();
		if(!forked)
		{
			close(childfd);
			handle_child_request(logFile, udpPort, payloadSize, packetSpacing, mode );
		}
		else if(forked == -1)
		{
			perror("Fork failed. Check resource monitor.\n");
			return;
		}


		close(childfd);
	}
	close(sockfd);
	exit(EXIT_SUCCESS);
}

void handle_child_request(FILE * logfileS, char* udpPort, char* payloadSize, char* packetSpacing, char* mode)
{
	
	sock_fd = create_socket_to_listen_on(udpPort);

	if (-1 == gettimeofday(&tv1, NULL)) {
		perror("resettimeofday: gettimeofday");
		exit(-1);
	}
	while(state != READY)
	{

	}
	state = BUSY;
	while(state == BUSY)
	{
		printf("Still here packetSpacing %d \n", pktSpcing);
		do_sleep(pktSpcing*1000);
	}
	printf("Outta here \n");
	if (-1 == gettimeofday(&tv2, NULL)) {
		perror("resettimeofday: gettimeofday");
		exit(-1);
	}
	long ms = getTimeDifference(&tv1,&tv2);
	double time_duration = ms/1000.0;
	printf("Completion time: %lf seconds\n", time_duration);
	fclose(logfileS);
}


int create_socket_to_listen_on(char *rand_port)
{
	int  sd;
	struct sockaddr_in server;
	struct sockaddr_in foo;
	int len = sizeof(struct sockaddr);
	char buf[512];
	int rc;
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(atoi(rand_port));
	sd = socket (AF_INET, SOCK_DGRAM, 0);
	bind ( sd, (struct sockaddr *) &server, sizeof(server));
	getsockname(sd, (struct sockaddr *) &foo, &len);
	int sockfd = sd;
	int flags = 0;
	if (-1 == (flags = fcntl(sockfd, F_GETFL, 0)))
		flags = 0;
	if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0)
	{
		perror("Cant' make socket non blocking");
		exit(EXIT_FAILURE);
	}
	if (-1 == (flags = fcntl(sockfd, F_GETFL, 0)))
		flags = 0;
	if (fcntl(sockfd, F_SETFL, flags | O_ASYNC) < 0)
	{
		perror("Can't make socket asynchronous");
		exit(EXIT_FAILURE);
	}
	if (fcntl(sockfd, F_SETOWN, getpid()))
	{
		perror("Can't own the socket");
		exit(EXIT_FAILURE);
	}
	return sd;
}

size_t recv_socket_data(int in_fd, char * buffer)
{
	struct sockaddr_in from;
	size_t length=sizeof(struct sockaddr_in);
	int n = recvfrom(in_fd,buffer,pySz,0,(struct sockaddr *)&from, &length);
	if (n < 0) error("recvfrom");
	return n;
}


void do_sleep(useconds_t time)
{
    struct sigaction sa;
    
    sigset_t mask;
    
    sa.sa_handler = &handle_sigalrm; // Intercept and ignore SIGALRM
    sa.sa_flags = SA_RESTART;
    sigfillset(&sa.sa_mask);
    sigaction(SIGALRM, &sa, NULL);
    
    // Get the current signal mask
    sigprocmask(0, NULL, &mask);

    // Unblock SIGALRM
    sigdelset(&mask, SIGALRM);

    // Wait with this mask
    ualarm(time, 0);
    sigsuspend(&mask);

    //printf("sigsuspend() returned\n");
}