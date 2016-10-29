
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
#include <gtk/gtk.h>

// Instructions: 
// gcc -o gui_wetalk gui_wetalk.c $(pkg-config --cflags --libs gtk+-2.0)
// follow same procedure as lab4 wetalk.c

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
void printfToMyMessageBar(__const char *__restrict __format);
void printfToTheirMessageBar(__const char *__restrict __format);

#ifndef	INET4ADDRLEN
#define	INET4ADDRLEN	sizeof(struct in_addr)
#endif

#define WANNATALK_CHK "WANNATALK"
#define OK_CHK "OK"
#define E_CHK "E"
#define NKO_CHK "KO"

#define NO_STATUS 1
#define INITIATOR 2
#define RECEIVER 3

volatile bool connection = false;
volatile bool received = true;
volatile bool alive = true;

struct  hostent *hp, *gethostbyname();

GtkBuilder *builder;
GtkWidget * window;
GtkWidget * button;
GtkWidget * messageBar1;
GtkWidget * messageBar2;
GtkWidget *	textBar;

pid_t current_pid = -1 ;
char term_buf[51] = {0};

volatile struct	sockaddr_in * opp_server_pointer = NULL;
struct sockaddr_in their_addr;
socklen_t addr_len;
int sockfd;

int get_in_port(struct sockaddr * sa)
{
	return (int)(((struct sockaddr_in*)sa)->sin_port);
}

void buttonClickedEvent(GtkWidget *w, gpointer p);

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
		received = true;
	errno = saved_errno;
}

int main(int argc, char** argv)
{
	their_addr.sin_family = AF_INET;
	addr_len = sizeof (their_addr);

	current_pid = getpid();

	char* udpPort;

	if (argc != 2)
	{
		fprintf(stderr, "usage: %s UDP_port\n\n", argv[0]);
		exit(1);
	}

	udpPort = argv[1];

	startWeChatServer(udpPort);

	GError    *error = NULL;

	gtk_init( &argc, &argv);

	// Widget : main window
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "WeTalk");
	gtk_window_set_default_size(GTK_WINDOW(window), 300, 500);
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

	GtkWidget *layout = gtk_fixed_new();

	textBar = gtk_entry_new();
	gtk_widget_set_size_request(textBar, 270, 30);

	button = gtk_button_new_with_label("Send");
	gtk_widget_set_size_request(button, 90, 30);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(buttonClickedEvent), NULL);

	messageBar1 = gtk_label_new("");
	gtk_widget_set_size_request(messageBar1, 290, 200);

	messageBar2 = gtk_label_new("");
	gtk_widget_set_size_request(messageBar2, 290, 200);

	gtk_fixed_put(GTK_FIXED(layout), textBar, 10, 450);
	gtk_fixed_put(GTK_FIXED(layout), button, 300, 450);
	gtk_fixed_put(GTK_FIXED(layout), messageBar1, 10, 10);
	gtk_fixed_put(GTK_FIXED(layout), messageBar2, 10, 145);

	gtk_container_add(GTK_CONTAINER(window), layout);

	gtk_widget_show_all(window);

	gtk_main();
	return 0;
}

void initialize_gui()
{

}



// Get the address if ipv4 or ipv6 although we only pertain with ipv4
char *get_in_addr(struct sockaddr *sa) {
	return (char *) inet_ntoa((((struct sockaddr_in*)sa)->sin_addr));
}


void buttonClickedEvent(GtkWidget *w, gpointer p) {

	char bufToSend[52] = {0};
	char toProcess[51] = {0};
	strncpy(toProcess, gtk_entry_get_text(GTK_ENTRY(textBar)), 51);
	if (toProcess[0] != 0)
	{


		sprintf(bufToSend, "%s \n", toProcess);
		printfToMyMessageBar(bufToSend);

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
					alive = false;
					alarm(0);
					close(sockfd);
					kill(current_pid, SIGKILL);
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
						return;
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

void printfToMyMessageBar(__const char *__restrict __format)
{
	char aux[500];
	char msg[52] = {0};
	strcpy(aux, gtk_label_get_text(GTK_LABEL(messageBar1)));
	strncpy(msg, __format, strlen(__format));
	strcat(aux, msg);
	gtk_label_set_text(GTK_LABEL(messageBar1), aux);
}


void printfToTheirMessageBar(__const char *__restrict __format)
{
	char aux[500];
	char msg[52] = {0};
	strcpy(aux, gtk_label_get_text(GTK_LABEL(messageBar2)));
	strncpy(msg, __format, strlen(__format));
	strcat(aux, msg);
	gtk_label_set_text(GTK_LABEL(messageBar2), aux);
}

// Start the traffic udp server
void startWeChatServer(char* myUDPport)
{



	opp_server.sin_family = AF_INET;
	// initalize the alarm stuff

	signal(SIGALRM, (void (*)(int))handle_signal_alarm);


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
		char buffToPrint[52]; // buff is large enough to hold the entire formatted string
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
				int opp_port =  ntohs(get_in_port((struct sockaddr *)&their_addr));
				opp_server_pointer = (struct sockaddr_in *)&their_addr;
				sprintf(buffToPrint, "| chat request from %s %d \n", opp_hostname , opp_port);
				printfToTheirMessageBar(buffToPrint);
			}
			if (!received)
			{
				if (!strcmp(OK_CHK, buf))
				{
					received = true;
					connection = true;

				}
				else if (!strcmp(NKO_CHK, buf))
				{
					received = true;
					connection = false;
					opp_server_pointer = NULL;
					sprintf(buffToPrint, "%s", "| doesn't want to chat \n");
					printfToTheirMessageBar(buffToPrint);
				}
			}
		}
		else
		{
			if (!strcmp(E_CHK, buf))
			{

				connection = false;
				opp_server_pointer = NULL;
				sprintf(buffToPrint, "%s", "| chat terminated \n");
				printfToTheirMessageBar(buffToPrint);
			}
			else if (strlen(buf) > 0 && buf[0] == 'D')
			{

				const char * first_string = "";
				const char * second_string = buf;
				char final_string[52];
				strcpy(final_string, first_string);     // copy to destination
				strcat(final_string, second_string + 1); // append part of the second string
				sprintf(buffToPrint, "| %s \n", final_string);
				printfToTheirMessageBar(buffToPrint);
			}
		}
	}
}

int startListeningOnPort(char * myUDPport)
{
	struct addrinfo hints, *servinfo, *p;
	int rv;
	struct sockaddr_storage their_addr;
	socklen_t addr_len;
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




