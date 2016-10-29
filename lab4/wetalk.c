
/*
* wetalk.c
*
*  Created on: Sep 17, 2016
*      Author: prashantravi
*/

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <wait.h>
#include <termios.h>
#include <stdbool.h>

void packet_handler();
int startListeningOnPort(char * myUDPport);
void startWeChatServer(char* myUDPport);
long getTimeDifference(struct timeval *t1 , struct timeval *t2);
void handle_signal_alarm(int sig);
bool isnumber(const char*s);

bool connection = false;
bool received = false;
int got_interrupt = 0;
void handle_signal_alarm(int sig)
{
	int saved_errno = errno;
	if (sig != SIGALRM) {
		perror("Caught wrong signal\n");
	}
	if(!received){
		printf( "no response from wetalk server\n" );
	}
	errno = saved_errno;
}

int main(int argc, char** argv)
{
	char* udpPort;

	if (argc != 2)
	{
		fprintf(stderr, "usage: %s UDP_port\n\n", argv[0]);
		exit(1);
	}

	udpPort = argv[1];
	startWeChatServer(udpPort);
	return 0;
}
// Get the address if ipv4 or ipv6 although we only pertain with ipv4
void *get_in_addr(struct sockaddr *sa) {
	return sa->sa_family == AF_INET
	       ? (void *) & (((struct sockaddr_in*)sa)->sin_addr)
	       : (void *) & (((struct sockaddr_in6*)sa)->sin6_addr);
}
// Start the traffic udp server
void startWeChatServer(char* myUDPport)
{
	
	int sockfd;

	// initalize the alarm stuff

	signal(SIGALRM,(void (*)(int))handle_signal_alarm);

	// initialize the terminal stuff

	static struct termios oldt, newt;

 
    tcgetattr( STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON|ECHO);          
    tcsetattr( STDIN_FILENO, TCSANOW, &newt);

	// initiate the packet handler stuff

	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	
	sa.sa_sigaction = &packet_handler;
	sa.sa_flags = SA_SIGINFO;

	if(!((sigaction(SIGIO, &sa, NULL ) == 0) && (sigaction(SIGPOLL, &sa, NULL) == 0)))
	{
		perror("Can't create signal action.");
		exit(EXIT_FAILURE);
	}

	if((sockfd = startListeningOnPort(myUDPport)) < 0)
	{
		perror("Can't create socket.");
		exit(EXIT_FAILURE);
	}


	if(fcntl(sockfd, F_SETFL,  O_NONBLOCK) < 0)
	{
		perror("Cant' make socket non blocking");
		exit(EXIT_FAILURE);
	}

	if(fcntl(sockfd, F_SETFL, O_ASYNC) < 0)
	{
		perror("Can't make socket asynchronous");
		exit(EXIT_FAILURE);
	}

	if(fcntl(sockfd, F_SETOWN, getpid()))
	{
		perror("Can't own the socket");
		exit(EXIT_FAILURE);
	}


	printf("?\n");

	while(1)
	{
		char r;
		char term_buf[51] ;
		int num_chars_read = 0;
		while( (r = getchar()) != '\n' && num_chars_read <=50)
		{
			if((r == 127 || r == 8) && num_chars_read >0)
			{
				term_buf[num_chars_read--] = '\0';
			}
			else
			{
				term_buf[num_chars_read++] = r ;
			}
		}
		term_buf[num_chars_read] = '\0';
		if(!connection)
		{
			char *p;
			char * hostname;
			char * port;
    		hostname = strtok(term_buf, " ");
    		if(!hostname) continue; 
    		port = strtok(NULL, ",");
   		   	if(!port || !isnumber(port)) continue;

   		   	// Make a connection
   		   	char *wannatalk = "WANNATALK";
   		   	if(sendto(sockfd, wannatalk, strlen(wannatalk), MSG_DONTWAIT , availableServerSockets->ai_addr, availableServerSockets->ai_addrlen) == -1)
			{
				perror("sendto: failed\n");
			}
			alarm(7);
        }
		else
		{
			printf("?\n");
		}
	



	}
	
	close(sockfd);
}


bool isnumber(const char*s) {
   char* e = NULL;
   (void) strtol(s, &e, 0);
   return e != NULL && *e == (char)0;
}

void packet_handler()
{

	printf("Packet handler was called . \n");

}

int startListeningOnPort(char * myUDPport)
{
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
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
	for (p = servinfo; p != NULL; p = p->ai_next) {
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
	freeaddrinfo(servinfo);
	return sockfd;
}

void  establish_connection()
{

}




