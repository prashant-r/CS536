#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>


typedef int bool;
#define true 1
#define false 0

#define MAXIMUM_WAITING_CLIENTS 10

#define MAX_BUF 1024

void handle_sigchld(int sig);
void handle_sigchld(int sig) {
	int saved_errno = errno;
	while (waitpid((pid_t)(-1), 0, WNOHANG|WUNTRACED) > 0) {}
	errno = saved_errno;
}

int setUpHalfAssociation(char * PORT);
int setUpHalfAssociation(char * PORT)
{
	int halfAssociationSocketfd = -1;
	struct addrinfo prepInfo;
	struct addrinfo *availableSocketAddressesStructs;

	memset(&prepInfo, 0, sizeof prepInfo);
	prepInfo.ai_family = AF_INET;
	prepInfo.ai_socktype = SOCK_STREAM;
	prepInfo.ai_flags = AI_PASSIVE;

	if ((getaddrinfo(NULL, PORT, &prepInfo, &availableSocketAddressesStructs)) != 0) {//If it fails...
		return -1;
	}

	while( availableSocketAddressesStructs != NULL)
	{
		bool error = false;

		if ((halfAssociationSocketfd = socket(availableSocketAddressesStructs->ai_family, availableSocketAddressesStructs->ai_socktype,
				availableSocketAddressesStructs->ai_protocol)) == -1) {
			error = true;
		}
		int t = 1;
		if (setsockopt(halfAssociationSocketfd, SOL_SOCKET, SO_REUSEADDR, &t,sizeof(int)) == -1) {
			exit(EXIT_FAILURE);
		}

		if (bind(halfAssociationSocketfd, availableSocketAddressesStructs->ai_addr, availableSocketAddressesStructs->ai_addrlen) == -1)
		{
			close(halfAssociationSocketfd);
			error = true;
		}

		if(error)
		{
			availableSocketAddressesStructs = availableSocketAddressesStructs->ai_next;
		}
		else
		{
			break;
		}
	}
	if (availableSocketAddressesStructs == NULL)  {
		return -1;
	}

	freeaddrinfo(availableSocketAddressesStructs);
	return halfAssociationSocketfd;
}

void validateCommandLineArguments(int argc, char * argv[]);
void validateCommandLineArguments(int argc, char * argv[])
{
	if(argc != 4)
	{
		printf("Check number of arguments (expected 3 , got %d) - Usage portnumber secretkey configfile.dat\n", argc);
		exit(1);
	}
}

void validateConfigFile(char * filename);
void validateConfigFile(char * filename)
{

	FILE * fp;
	fp = fopen (filename, "r");
	rewind(fp);
	fscanf(fp, "%d", &MAX_DATA_AT_ONCE);
	fclose(fp);
	NUMPACKETS = (FILE_SIZE_IN_BYTES + MAX_DATA_AT_ONCE - 1) / MAX_DATA_AT_ONCE;
}

void validateSecretKey(char * secretKey);
void validateSecretKey(char * secretKey)
{
	size_t secretKey_len = strlen(secretKey);

	if(secretKey_len <10 || secretKey_len > 20 )
	{
		perror("Secret Key must be at least length 10 but not more than 20");
		exit(1);
	}
	for (i=0; i <= secretKey_len;i++)
	{
		if (isascii(secretKey[i]) == 0 )
		{
			perror("Secret Key must be ASCII representable.");
			exit(1);
		}
	}
}

int main(int argc, char * argv[])
{


	// Validate the command line arguments
	validateCommandLineArguments(argc, argv);

	int halfAssocSockfd;// socket file descriptor to create half association
	int fullAssocSockfd;// socket file descriptor to create full association

	halfAssocSockfd = setUpHalfAssociation(argv[1]);


	char * secretKeyGiven = argv[2];

	if(halfAssocSockfd == -1)
	{
		printf("\n Failed to set up half association \n");
		return 2;
	}

	printf("Server ON.\n");

	if (listen(halfAssocSockfd, MAXIMUM_WAITING_CLIENTS) == -1) {//If it fails...
		close(halfAssocSockfd);
		exit(EXIT_FAILURE);
	}

	char clientAddress[INET6_ADDRSTRLEN];
	signal(SIGCHLD, handle_sigchld);
	while (1)
	{
		socklen_t sin_size = sizeof clientAddress;
		fullAssocSockfd = accept(halfAssocSockfd, (struct sockaddr *)&clientAddress, &sin_size);

		if(fullAssocSockfd == -1)
		{
			close(fullAssocSockfd);
			close(halfAssocSockfd);
			exit(EXIT_FAILURE);
		}
		int child;

		if (!(child =fork()))
		{
			// this is the child process
			close(halfAssocSockfd);

			char buf[BUFSIZ];

			// Read the response
			int numReadBytes;
			if ((numReadBytes = read(fullAssocSockfd, buf, sizeof(buf)-1)) > 0)
			{
				buf[numReadBytes] = '\0';
			}

			// Decode the message
			char *saveptr;
			char *secretKey, *command;
			secretKey = strtok_r(buf, "$", &saveptr);
			command = strtok_r(NULL, "$", &saveptr);

			if(strcmp(secretKey, secretKeyGiven ) == 0)
			{
			if (strcmp(command, "ls") == 0 || strcmp(command, "cal") == 0 || strcmp(command, "date") == 0 || strcmp(command, "host") == 0)
			{
				dup2(fullAssocSockfd,STDOUT_FILENO);
				close(fullAssocSockfd);
				if(execlp(command,command,NULL) == -1)
				{

				}
			}
			else
			{
				printf("You have entered an invalid command \n");
			}
			}
			else
			{
				printf("Invalid secret key \n");
			}
			exit(EXIT_SUCCESS);
		}
		close(fullAssocSockfd);

	}
	close(halfAssocSockfd);
	return EXIT_SUCCESS;
}




