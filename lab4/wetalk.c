
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
#include <stdio_ext.h>


void packet_handler();
int startListeningOnPort(char * myUDPport);
void startWeChatServer(char* myUDPport);
long getTimeDifference(struct timeval *t1 , struct timeval *t2);
void handle_signal_alarm(int sig);
bool isnumber(const char*s);
void write_stdout_out();
void empty_stdout_out();
void refresh_stdout_out();
char *get_in_addr(struct sockaddr *sa);
int get_in_port(struct sockaddr * sa);

#ifndef	INET4ADDRLEN
#define	INET4ADDRLEN	sizeof(struct in_addr)
#endif

struct sockaddr_in their_addr;
socklen_t addr_len;

#define WANNATALK_CHK "WANNATALK"
#define OK_CHK "OK"
#define E_CHK "E"
#define NKO_CHK "KO"

#define NO_STATUS 1
#define INITIATOR 2
#define RECEIVER 3

volatile bool connection = false;
volatile bool received = false;
char term_buf[51] = {0};

volatile struct	sockaddr_in * opp_server_pointer = NULL;

int sockfd;

int get_in_port(struct sockaddr * sa)
{
	return (int)(((struct sockaddr_in*)sa)->sin_port);
}



struct sockaddr_in opp_server;

void handle_signal_alarm(int sig)
{
	int saved_errno = errno;
	if (sig != SIGALRM) {
		perror("Caught wrong signal\n");
	}
	if (!received) {
		printf( "no response from wetalk server\n" );
		connection = false;
		opp_server_pointer = false;
		fflush(stdout);
	}
	else
		received = false;
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
char *get_in_addr(struct sockaddr *sa) {
 	return (char *) inet_ntoa((((struct sockaddr_in*)sa)->sin_addr));
}
// Start the traffic udp server
void startWeChatServer(char* myUDPport)
{

	their_addr.sin_family = AF_INET;
	addr_len = sizeof (their_addr);
	
	struct  hostent *hp, *gethostbyname();
	opp_server.sin_family = AF_INET;
	// initalize the alarm stuff

	signal(SIGALRM, (void (*)(int))handle_signal_alarm);

	// initialize the terminal stuff

	static struct termios oldt, newt;


	tcgetattr( STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr( STDIN_FILENO, TCSANOW, &newt);

	// initiate the packet handler stuff

	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));

	sa.sa_sigaction = &packet_handler;
	sa.sa_flags = SA_SIGINFO;

	if (!((sigaction(SIGIO, &sa, NULL ) == 0) && (sigaction(SIGPOLL, &sa, NULL) == 0)))
	{
		perror("Can't create signal action.");
		exit(EXIT_FAILURE);
	}

	if (startListeningOnPort(myUDPport) < 0)
	{
		perror("Can't create socket.");
		exit(EXIT_FAILURE);
	}
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
	while (1)
	{
		char r;
		
		int num_chars_read = 0;
		memset(term_buf, 0, sizeof term_buf);

		if(connection)
		{
			empty_stdout_out();
			printf(">");
			fflush(stdout);
		}
		else
		{
			empty_stdout_out();
			printf("?");
			fflush(stdout);
		}
		
		while ( (r = getchar()) != '\n' && num_chars_read <= 50)
		{
			if ((r == 127 || r == 8) && num_chars_read > 0)
			{
				fputc('\b',stdout);
				fputc(' ',stdout);
				term_buf[--num_chars_read] = '\0';
			}
			else if (isprint(r) && !(r == 127 || r == 8))
			{
				term_buf[num_chars_read++] = r ;
			}

			if(num_chars_read == 0)
			{
				memset(term_buf, 0, sizeof term_buf);
			}
			refresh_stdout_out();
		}
		term_buf[num_chars_read] = '\0';
		char toProcess[51] = {0};
		strncpy(toProcess, term_buf, 51);
		write_stdout_out();
		
		if (toProcess[0] != 0)
		{
			if (!connection)
			{
				if (opp_server_pointer != NULL)
				{
					if (!strcmp("c", toProcess))
					{
						char * OK = "OK";
						if (sendto(sockfd, OK, strlen(OK), MSG_DONTWAIT , (struct sockaddr *) opp_server_pointer, sizeof(*opp_server_pointer)) == -1)
						{
							perror("sendto OK: failed\n");
						}
						connection = true;
					}
					else if (!strcmp("n", toProcess))
					{
						char * KO = "KO\0";
						if (sendto(sockfd, KO, strlen(KO), MSG_DONTWAIT , (struct sockaddr *) opp_server_pointer, sizeof(*opp_server_pointer)) == -1)
						{
							perror("sendto KO: failed\n");
						}
						connection = false;
						opp_server_pointer = NULL;
					}
					else
					{

					}
				}
				else
				{
					if (!strcmp("q", toProcess))
					{
						break;
					}
					else
					{
						char *p;
						char * hostname;
						char * port;
						hostname = strtok(toProcess, " ");
						if (!hostname) goto ILLEGAL_HOSTNAME_PORT;
						port = strtok(NULL, ",");
						if (!port || !isnumber(port)) goto ILLEGAL_HOSTNAME_PORT;

						// Make a connection
						char *wannatalk = "WANNATALK";
						hp = gethostbyname(hostname);
						if (hp != NULL)
						{
							bcopy ( hp->h_addr, &(opp_server.sin_addr.s_addr), hp->h_length);
							opp_server.sin_port = htons(atoi(port));
							opp_server_pointer = &opp_server;
							if (sendto(sockfd, wannatalk, strlen(wannatalk), MSG_DONTWAIT , (struct sockaddr *) &opp_server, sizeof(opp_server)) == -1)
							{
								perror("sendto wannatalk: failed\n");
							}
							received = false;
							alarm(7);
						}
						else
						{
ILLEGAL_HOSTNAME_PORT:
							printf("ERROR: Invalid wetalk peer. Usage : hostname port\n");
							continue;
						}
					}
				}
			}
			else
			{
				if (opp_server_pointer != NULL)
				{
					if (!strcmp("e", toProcess))
					{
						char msg[1] = {0};
						msg[0] = 'E';
						if ( sendto(sockfd, msg, sizeof msg, 0, (struct sockaddr*)opp_server_pointer, sizeof(*opp_server_pointer)) == -1)
						{
							perror("sendto e: failed\n");
						}
						connection = false;
						opp_server_pointer = NULL;
						
					}
					else
					{
						char msg[ 52 ] = { 0 };
						strcat(msg, "D" );
						strcat(msg, toProcess);
						if ( sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr*)opp_server_pointer, sizeof(*opp_server_pointer)) == -1)
						{
							perror("sendto msg: failed\n");
						}
					}
				}
				else
				{
					printf("ERROR: Wetalk wrongly assumes connection.\n");
				}
			}
		}
	}
	alarm(0);
	tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
	close(sockfd);
}

void empty_stdout_out()
{
	fputs("\r", stdout);
}

void refresh_stdout_out()
{
	fputs("\r", stdout);
	fprintf(stdout, "%s", term_buf);
}

void write_stdout_out()
{
	fputc('\n', stdout);
}


bool isnumber(const char*s) {
	char* e = NULL;
	(void) strtol(s, &e, 0);
	return e != NULL && *e == (char)0;
}

void packet_handler()
{
	char buf[52] = {0};
	char s[INET4ADDRLEN];
	int rc;
	while (1)
	{
		rc = recvfrom (sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&their_addr, &addr_len);
		their_addr.sin_family = AF_INET;
		if (rc <= 0)
		{
			break;
		}
		buf[rc] = '\0';
		if (!connection)
		{
			if (!strcmp(WANNATALK_CHK, buf))
			{
				char * opp_hostname = get_in_addr((struct sockaddr *)&their_addr);
				int opp_port =  get_in_port((struct sockaddr *)&their_addr);
				opp_server_pointer = (struct sockaddr_in *)&their_addr;
				empty_stdout_out();
				printf("| chat request from %s %d \n?", opp_hostname , opp_port);
				
			}
			else if (!strcmp(OK_CHK, buf))
			{
				received = true;
				connection = true;
				empty_stdout_out();
				printf(">");
				
			}
			else if (!strcmp(NKO_CHK, buf))
			{
				received = true;
				connection = false;
				empty_stdout_out();
				printf("%s\n?", "| doesn't want to chat");
			}
		}
		else
		{
			if (!strcmp(E_CHK, buf))
			{
				
				connection = false;
				opp_server_pointer = NULL;
				printf("%s\n?", "| chat terminated");
			}
			else if (strlen(buf) > 0 && buf[0] == 'D')
			{

				const char * first_string = "";
				const char * second_string = buf;
				char final_string[52];
				strcpy(final_string, first_string);     // copy to destination
				strcat(final_string, second_string + 1); // append part of the second string
				empty_stdout_out();
				printf("| %s", final_string);
				int num_spaces_to_print = 55 - strlen(final_string);
				// Overwrite previous characters
				while(num_spaces_to_print-- >0)
				{
					printf(" ");
				} 
				printf("\n>");
				refresh_stdout_out();
			}
		}
	}
}

int startListeningOnPort(char * myUDPport)
{
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET4ADDRLEN];

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




