/*
* audiolisten.c
* Created on: Nov 15, 2016
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

#include <strings.h>
#include <unistd.h>
#include <linux/soundcard.h>
#include <sys/ioctl.h>

#define BUFSIZE 1024
#define AUDIODEVICE "/dev/sdb1"
#define AUDIOMODE O_RDWR

int create_socket_to_listen_on(char *rand_port);
size_t send_socket_data(int in_fd, char * message, struct sockaddr * server_addr);
size_t recv_socket_data(int in_fd, char * buffer);
void packet_handler();
void playback_handler();
int open_audio();
void close_audio(int fd);
int pyld_sz;
int pl_del;
int sfd;
int audio_fd;
struct timeval startWatch;
/* 
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}

volatile bool playing = false;


int open_audio()
{
    int arg, err, fd=-1;
    struct stat st;


    /* stat the device */
    err = stat(AUDIODEVICE, &st);
    if (err < 0)
    {
        fprintf(stderr,"cannot stat audio device");
        exit(1);
    }
    /*  verify if is a character device */
    if (!S_ISCHR(st.st_mode))
    {
        fprintf(stderr,"%s is not an audio device\n", AUDIODEVICE);
        exit(1);
    }
    /* try to open the device, otherwise*/
    fd= open(AUDIODEVICE, AUDIOMODE /* | O_NONBLOCK */);
    if ((fd < 0) && (errno == EBUSY))
    {
        printf ("%s is busy\nwaiting...\n", AUDIODEVICE);

        /* Now hang until it's open */
        /* blocking open will wait until other applications stop using it */
        fd= open(AUDIODEVICE, AUDIOMODE);
    }

    if (fd < 0)
    {
        printf("error opening audio device");
        exit(1);
    }

    arg=0x00020008;

    if (ioctl(fd, SNDCTL_DSP_SETFRAGMENT, &arg))
        printf("audio_open: ioctl\n");

    return fd;
}

void close_audio(int fd)
{
    close(fd);
}

void playback_handler()
{


}


void packet_handler()
{
    char buffer[pyld_sz];
    int n = 0;
    n = recv_socket_data(sfd, buffer);
    if( n > 0)
    {
        // Some data coming my way.
        printf("Data coming \n");
        write(audio_fd, buffer, sizeof(buffer));
    }
    else if(n == 0)
    {
        // End of transmission 
        printf("End of transmission \n");
        playing = false;
    }
    else
    {
        perror("recvfrom: error in packet handler.\n");
    }
}

int main(int argc, char **argv) {
    int sockfd, portno, n;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char * hostname;
    char * client_udp_port;
    char * payload_size;
    char * playback_del;
    char * gamma;
    char * buf_sz;
    char * target_buf;
    char * logfile_c;
    char * filename; 

    char buf[BUFSIZE];

    /* check command line arguments */
    if (argc != 11) {
       fprintf(stderr,"usage: %s server-ip server-tcp-port client-udp-port payload-size playback-del gamma buf-sz target-buf logfile-c filename(short or long)\n", argv[0]);
       exit(0);
    }
    hostname = argv[1];
    portno = atoi(argv[2]);
    client_udp_port = argv[3];
    payload_size = argv[4];
    pyld_sz = atoi(payload_size);
    playback_del = argv[5];
    pl_del = atoi(playback_del);
    gamma = argv[6];
    buf_sz = argv[7];
    target_buf = argv[8];
    logfile_c = argv[9];
    filename = argv[10];

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
	  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);

    /* connect: create a connection with the server */
    if (connect(sockfd, &serveraddr, sizeof(serveraddr)) < 0) 
      error("ERROR connecting");


    // sigio or sigpoll
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = &packet_handler;
    sa.sa_flags = SA_SIGINFO;
    if (!((sigaction(SIGIO, &sa, NULL ) == 0) && (sigaction(SIGPOLL, &sa, NULL) == 0)))
    {
        perror("Can't create io signal action.");
        exit(EXIT_FAILURE);
    }

    // create socket to listen on 
    sfd = create_socket_to_listen_on(client_udp_port);

    /* construct the message for the user */
    bzero(buf, BUFSIZE);
    char port_path[50];
    bzero(port_path, 50);

    strcat(port_path, client_udp_port);
    strcat(port_path, " ");
    strcat(port_path, filename);
    strcpy(buf, port_path);

    /* send the message line to the server */
    n = write(sockfd, buf, strlen(buf));
    if (n < 0) 
      error("ERROR writing to socket");

    /* print the server's reply */
    bzero(buf, BUFSIZE);
    n = read(sockfd, buf, BUFSIZE);
    if (n < 0) 
      error("ERROR reading from socket");
    printf("Echo from server: %s\n", buf);
    close(sockfd);

    /* open the audio device driver */
    audio_fd = open_audio();

    if (-1 == gettimeofday(&startWatch, NULL)) {
        perror("resettimeofday: gettimeofday");
        exit(-1);
    }
    playing = true;
    usleep(pl_del*1000);
    while(playing)
    {
        usleep(atoi(gamma)*1000);
        playback_handler();
    }
    return 0;
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



size_t send_socket_data(int in_fd, char * message, struct sockaddr * server_addr)
{
    size_t numSent;
    struct sockaddr recv_sock;
    socklen_t addr_len = sizeof(recv_sock);
    struct sockaddr * who_to_send_addr = server_addr;
    numSent = sendto(in_fd, message, strlen(message) , 0, who_to_send_addr, addr_len);
    return numSent;
}

size_t recv_socket_data(int in_fd, char * buffer)
{
    struct sockaddr_in from;
    size_t length=sizeof(struct sockaddr_in);
    int n = recvfrom(in_fd,buffer,pyld_sz,0,(struct sockaddr *)&from, &length);
    if (n < 0) error("recvfrom");
    return n;
}