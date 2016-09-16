#define _XOPEN_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

void handle_sigchld(int sig);

void handle_sigchld(int sig) {
	int saved_errno = errno;
	while (waitpid((pid_t)(-1), 0, WNOHANG|WUNTRACED) > 0) {}
	errno = saved_errno;
}

int main()
{
	int client_to_server;

	char *myfifo = "/tmp/cmdfifo";
	char *fcfifo = "/tmp/cfifo";

	char buf[BUFSIZ];

	mkfifo(myfifo, 0666);

	client_to_server = open(myfifo, O_RDONLY);

	printf("Server ON.\n");
	signal(SIGCHLD, handle_sigchld);
	while (1)
	{
		read(client_to_server, buf, BUFSIZ);
		if (strcmp("",buf)!=0)
		{
			int child;

			if (!(child =fork()))
			{
				// this is the child process
				printf("Received: %s\n", buf);

				// Decode the message
				char *saveptr;
				char *pid, *command;
				pid = strtok_r(buf, "$", &saveptr);
				command = strtok_r(NULL, "$", &saveptr);


				// Find the right cfifopid
				char cwd[1024];
				strcpy(cwd, fcfifo);
				char src[1024], cfifopid[1024];
				strcpy(src,  pid);
				strcpy(cfifopid, cwd);
				strcat(cfifopid, src);

				// Do something with the message
				int fw=open(cfifopid, O_APPEND|O_WRONLY);
				dup2(fw,1);
				if (strcmp(command, "ls") == 0)
				{
					if(execlp(command,command,NULL) == -1)
					{

					}
					close(fw);
				}
				else /* default: */
				{
					printf("You have entered an invalid command");
				}

				exit(EXIT_SUCCESS);
			}
		}
		/* clean buf from any data */
		memset(buf, 0, sizeof(buf));
	}

	close(client_to_server);

	unlink(myfifo);
	return 0;
}
