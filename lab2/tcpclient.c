#define _XOPEN_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

typedef int bool;
#define true 1
#define false 0

#define MAX_BUF 1024
#define CMD "ls"

void validateCommandLineArguments(int argc, char * argv[])
{
	if(argc != 3)
	{
		printf("Too few arguments\n");
		exit(1);
	}
}

int main(int argc, char * argv[])
{
	// Validate the command line arguments
	validateCommandLineArguments(argc, argv);

	// Create the sending buffer and the receiving buffer
	char recvBuffer[MAX_BUF];
	char sendBuffer[MAX_BUF];

	// Setup the socket
	struct addrinfo addressInfo;
	memset(&addressInfo, 0, sizeof addressInfo);

	addressInfo.ai_family = AF_INET; // Use IPv4
	addressInfo.ai_socktype = SOCK_STREAM;// Use TCP

	//  populate the address info
	struct addrinfo *availableServerSockets;
	int getAddrCheck;
	if ((getAddrCheck = getaddrinfo(argv[1], argv[2], &addressInfo, &availableServerSockets)) != 0) {//If it fails...
		printf("Failed to get address info for %s failed ", argv[1]);
		return -1;
	}

	int sockfd;
	struct addrinfo *nextAvailableServerSocket;
	bool connectionSuccessful = false;
	while(availableServerSockets != NULL)
	{
		bool error = false;

		if ((sockfd = socket(availableServerSockets->ai_family, availableServerSockets->ai_socktype,availableServerSockets->ai_protocol)) == -1) {//If it fails...
			error = true;
		}

		if (connect(sockfd, availableServerSockets->ai_addr, availableServerSockets->ai_addrlen) == -1) {
			error = true;
		}
		if(error)
			availableServerSockets = availableServerSockets->ai_next;
		else
		{
			connectionSuccessful = true;
			break;
		}
	}

	if(!connectionSuccessful)
	{
		printf("client: failed could not connect \n");
		return 2;
	}


	// Write the request
	snprintf(sendBuffer, sizeof(sendBuffer), "%s", CMD);
	write(sockfd, sendBuffer, strlen(sendBuffer));

	// Read the response
	int numReadBytes;
	while ( (numReadBytes = read(sockfd, recvBuffer, sizeof(recvBuffer)-1)) > 0)
	{
		recvBuffer[numReadBytes] = 0;
		if(fputs(recvBuffer, stdout) == EOF)
		{
			printf("\n Error : Fputs error\n");
		}
	}

	if(numReadBytes < 0)
	{
		printf("\n Error reading \n");
	}
	else if(numReadBytes == 0)
	{
		printf("\n Response was empty \n");
	}

	close(sockfd);
}
