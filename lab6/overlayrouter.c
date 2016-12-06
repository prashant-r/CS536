#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <sys/utsname.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <netdb.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <stdbool.h>

#define MAX_BUF 1000
#define EXPECTED_RESPONSE "terve"
#define TIME_OUT 550000
#define INTERVAL 50000
#define MAX_BUF 1000
#define MAX_PKT_LEN 8096

volatile bool received = false;
pid_t current_pid = -1 ;
int sendMyTunnelRequest(char* hostname, char* hostUDPport, char *rserver_host , char * rserver_port);
void validateCommandLineArguments(int argc, char ** argv);
long getTimeDifference(struct timeval *t1 , struct timeval *t2);

typedef struct neighborentry {
  char src_hostname[16];
  int src_port;
  char dest_hostname [16];
  int dest_port;
  bool confirmed;
  struct neighborentry * next;
} nbr_entry_t;
char *get_in_addr(struct sockaddr *sa);
in_port_t get_in_port(struct sockaddr *sa);

char *get_in_addr(struct sockaddr *sa) {
	return (char *) inet_ntoa((((struct sockaddr_in*)sa)->sin_addr));
}

in_port_t get_in_port(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return htons((((struct sockaddr_in*)sa)->sin_port));
    }

    return htons((((struct sockaddr_in6*)sa)->sin6_port));
}


int create_send_server_socket_data(int in_fd, char * hostname, char * hostUDPport);
size_t send_socket_data(int in_fd, struct sockaddr * server_addr);
int create_socket_to_listen_on(int *rand_port);
void startTunnelServer(char* myUDPport);
void registration_proc(char * myUDPport, int sockfd);
bool isValidIpAddress(char *ipAddress);
void handle_sigchld(int sig);

void handle_sigchld(int sig) {
	int saved_errno = errno;
	while (waitpid((pid_t)(-1), 0, WNOHANG|WUNTRACED) > 0) {}
       errno = saved_errno;
}

bool isnumber(const char*s);

bool isnumber(const char*s) {
   char* e = NULL;
   (void) strtol(s, &e, 0);
   return e != NULL && *e == (char)0;
}

int main(int argc, char** argv)
{
	char* udpPort;

	if (argc != 2)
	{
		fprintf(stderr, "usage: %s server-port\n\n", argv[0]);
		exit(1);
	}

	udpPort = argv[1];

	startTunnelServer(udpPort);
	return 0;
}
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
	registration_proc(myUDPport, sockfd);
	exit(EXIT_SUCCESS);
}


void registration_proc(char * myUDPport, int sockfd)
{

	int KNOWNN_OVERLAY_PORT = atoi(myUDPport);
	if(KNOWNN_OVERLAY_PORT<=0) return;
	struct sockaddr_storage their_addr;
	socklen_t addr_len ;
	addr_len = sizeof their_addr;
	signal(SIGCHLD, handle_sigchld);
	while (1) {
		int numbytes;
		struct sockaddr_storage new_addr;
		socklen_t new_addr_len;
		// blocking call to receive the traffic sent by traffic_snd.
		char buffer[BUFSIZ];
		if ((numbytes = recvfrom(sockfd, buffer, sizeof buffer, 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
			perror("recvfrom");
			return;
		}
		//nt_insert(-1, &their_addr, -1 , NULL)
		if (!fork())
		{
			int rand_port = 0;
			int local_listen_sock = create_socket_to_listen_on(&rand_port);
			struct	sockaddr_in server;
			struct  hostent *hp, *gethostbyname();
			server.sin_family = AF_INET;
			// Let mytunnel know what port has been assigned
			//	printf("Now send client data to server \n");
			char * pch;
			int ch = '$';
			int counter = strlen(buffer);
			pch = buffer +  counter -1;
			//printf("The buffer is %s \n", buffer);
			char *server_to_contact;
			int port_number;
			char ** ch_ptrs = malloc (sizeof (char * ) * 1000);
  			bzero(ch_ptrs, 1000);
  			//printf("Split \"%s\"\n", buffer);
  			ch_ptrs[0] = strtok(buffer, "$");
  			int a =0;
  			while (ch_ptrs[a] != NULL) {
  				//printf("%s\n", ch_ptrs[a]);
  				ch_ptrs[++a] = strtok(NULL, "$");
  			}
  			//printf("The value of a is %d \n", a);
  			char response[13];
			sprintf(response, "%d", rand_port);
  			if(a == 3 )
  			{
  				//printf("Reached the end \n");

  				struct sockaddr_in server;
				struct  hostent *hp, *gethostbyname();

				server.sin_family = AF_INET;

  				//printf("host name  %s | host port %d \n", ch_ptrs[0], ntohs(atoi(ch_ptrs[1])));	

				hp = gethostbyname(ch_ptrs[0]);
				if(hp != NULL)
				{
					bcopy ( hp->h_addr, &(server.sin_addr.s_addr), hp->h_length);
					server.sin_port = htons(atoi(ch_ptrs[1]));
					
					int n;
					n = sendto(local_listen_sock, response, sizeof response, 0, (struct sockaddr *) &their_addr, addr_len);
					if (n < 0)
					{
						perror("ERROR in sendto\n");
						goto EXIT_THIS_CHILD;
					}
					//	printf("host name  %s | host port %d \n", packet.server_ip, ntohs(server.sin_port));
					struct sockaddr * client_addr = NULL;

					char * from_host = get_in_addr((struct sockaddr *)&their_addr);
					int from_port = get_in_port((struct sockaddr *)&their_addr);
					char * to_host = get_in_addr((struct sockaddr *)&server);
					int to_port = get_in_port((struct sockaddr *)&server);
					time_t ltime; /* calendar time */
    				ltime=time(NULL); /* get current cal time */
					printf("\n Time %s | Neighbour dest update %s:%d --> %s:%d \n",asctime( localtime(&ltime) ) , from_host, from_port, ch_ptrs[0], atoi(ch_ptrs[1]));
					send_socket_data(local_listen_sock,(struct sockaddr*) &server);
				}

  			}
  			else
  			{
  				struct sockaddr_in from;
  				socklen_t addrlen = sizeof(from); /* must be initialized */
  				char str[280];
				strcpy(str, "$");
  				// strip last router ip 
  				char* server_to_contact = ch_ptrs[a-2];

  				// reconstruct message
  				int x;
  				for(x = 0; x < a; x++)
  				{
  					if(x > a-2)
  					{
  						continue;
  					}
  					else
  					{
						strcat(str, ch_ptrs[x]);
						strcat(str, "$");
  					}
  				}

  				// forward message to overlay router
  				hp = gethostbyname(server_to_contact);
				if(hp != NULL)
				{
					bcopy ( hp->h_addr, &(server.sin_addr.s_addr), hp->h_length);
					server.sin_port = htons(KNOWNN_OVERLAY_PORT);
				}
				//printf("Message to send the next router %s \n ", str);
  				int n;
  				n = sendto(local_listen_sock, str, strlen(str), 0, (struct sockaddr *) &server, addr_len);
				if (n < 0)
				{
						perror("ERROR in sendto\n");
				}
				char recv_response[13];
				bzero(recv_response, 13);
				n = recvfrom(local_listen_sock, recv_response, sizeof(recv_response), 0, (struct sockaddr *)&from, &addrlen);
				if(n < 0) {
						char invalid[1];
					    invalid[0] = 'A';
						n = sendto(local_listen_sock,invalid, sizeof(invalid),  0, (struct sockaddr *) &their_addr, addr_len);
						if(n < 0)
						{
							perror("ERROR in sendto \n");
						}
						goto EXIT_THIS_CHILD;
				}
				struct hostent *hostp; /* server host info */
				hostp = gethostbyaddr((const char *)&from.sin_addr.s_addr,sizeof(from.sin_addr.s_addr), AF_INET);
				if (hostp == NULL)
					perror("ERROR on gethostbyaddr");
				char *hostaddrp; /* dotted decimal server host addr string */
				hostaddrp = inet_ntoa(from.sin_addr);
				if(isnumber(recv_response ))
				{
					if(atoi(recv_response) == -1)
					{
						char invalid[1];
					    invalid[0] = 'A';
						n = sendto(local_listen_sock,invalid, sizeof(invalid),  0, (struct sockaddr *) &their_addr, addr_len);
						if(n < 0)
						{
							perror("ERROR in sendto \n");
						}
						goto EXIT_THIS_CHILD;
					}
					else{
						char * from_host = get_in_addr((struct sockaddr *)&from);
						int from_port = get_in_port((struct sockaddr *)&from);
						char * to_host = get_in_addr((struct sockaddr *)&server);
						int to_port = get_in_port((struct sockaddr *)&server);
						time_t ltime; /* calendar time */
    					ltime=time(NULL); /* get current cal time */
						printf("Time %s | Neighbour route update %s:%d --> %s:%d \n",asctime( localtime(&ltime) ) , from_host, from_port, server_to_contact, KNOWNN_OVERLAY_PORT);
						//printf("Success! Make requests on port : %s\n", recv_response);
						n = sendto(local_listen_sock, response, sizeof(response), 0, (struct sockaddr *) &their_addr, addr_len);
						if (n < 0)
						{
							perror("ERROR in sendto\n");
						}
						server.sin_port = htons(atoi(recv_response));
						//printf("Neighbour table update %s:%d --> %s:%d \n", their_addr.sin_addr.s_addr, their_addr.sin_port, server.sin_addr.s_addr, server.sin_port);
						send_socket_data(local_listen_sock,(struct sockaddr*) &server);
					}
				}
				else
				{
					if(recv_response[0] == 'A')
					{
						printf("Route NOT established after 30 second wait. \n");
					}
					else
					{
						printf("Packet received was corrupt : %s\n", recv_response);
					}
				}
  			}
			
			EXIT_THIS_CHILD:
				close(local_listen_sock);
				exit(EXIT_SUCCESS);
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
			//printf("Looping here \n");
			numRead = recvfrom(in_fd, buf, toRead, 0, &recv_sock, &addr_len);

			if(numRead == -1) continue;

			//printf("numread is %d\n", (int) numRead);
			client_addr = &recv_sock;

			char * opp_hostname = get_in_addr((struct sockaddr *)&recv_sock);
			int opp_port =  ntohs(get_in_port((struct sockaddr *)&recv_sock));

			char * opp_hostname_chk = get_in_addr((struct sockaddr *)server_addr);
			int opp_port_chk =  ntohs(get_in_port((struct sockaddr *)server_addr));

			//	printf("LHS1 is %s %d \n", opp_hostname, opp_port);
			//	printf("RHS1 is %s %d \n", opp_hostname_chk, opp_port_chk);
			if(!strcmp(opp_hostname, opp_hostname_chk) && opp_port == opp_port_chk)
			{
				printf("No client so server can't send to anyone. \n ");
				continue;
			}

			who_to_send_addr = server_addr;
		}
		else
		{
			//printf("Or here \n");
			numRead = recvfrom(in_fd, buf, toRead, 0 , &test_recv_sock, &addr_len);
			
			if(numRead == -1) continue;

			//	printf("numread is %d\n", (int) numRead);
			char * opp_hostname = get_in_addr((struct sockaddr *)&test_recv_sock);
			int opp_port =  ntohs(get_in_port((struct sockaddr *)&test_recv_sock));

			char * opp_hostname_chk = get_in_addr((struct sockaddr *)server_addr);
			int opp_port_chk =  ntohs(get_in_port((struct sockaddr *)server_addr));


			//	printf("LHS2 is %s %d \n", opp_hostname, opp_port);
			//	printf("RHS2 is %s %d \n", opp_hostname_chk, opp_port_chk);

			if(!strcmp(opp_hostname, opp_hostname_chk) && opp_port == opp_port_chk)
			{
			//		printf("who to send is client \n");
				who_to_send_addr = client_addr;
			}
			else
			{
		//		printf("who to send is server \n");
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

	struct timeval tv;
	tv.tv_sec = 30;
	tv.tv_usec = 0;
	if (setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
    	perror("Error");
	}
	if (-1 == (flags = fcntl(sockfd, F_GETFL, 0)))
		flags = 0;

	if (fcntl(sockfd, F_SETFL, flags) < 0)
	{
		perror("Cant' make socket non blocking");
		exit(EXIT_FAILURE);
	}
	return sd;
}