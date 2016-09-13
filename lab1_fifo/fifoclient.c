#define _XOPEN_SOURCE

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>


#define MAX_BUF 1024
#define CMD "ls"

int main()
{
   int client_to_server;
   int server_to_client;
   char *myfifo = "/tmp/cmdfifo";
   char *fcfifo = "/tmp/cfifo";

   // Construct the receiving fifo path
   char cfifo_path[MAX_BUF];
   strcpy(cfifo_path, fcfifo);
   char str_pid[MAX_BUF];
   sprintf(str_pid, "%d", getpid());
   strcat(cfifo_path, str_pid);
   fcfifo = cfifo_path;


   // Start the recieving client
   mkfifo(fcfifo, 0666);
   
   // Construct message
   char str[BUFSIZ];
   sprintf(str, "$%d$", getpid());
   strcat(str, CMD);

   // Send to server
   client_to_server = open(myfifo, O_WRONLY);
   write(client_to_server, str, sizeof(str));
   close(client_to_server);

   //Read from own fifo
   server_to_client = open(fcfifo, O_RDONLY);
   memset(str, 0, sizeof(str));
   read(server_to_client,str,sizeof(str));
   printf("\nServer Response:\n\n%s\n", str);

   return 0;
}
