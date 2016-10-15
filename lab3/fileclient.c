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
#include <ctype.h>

typedef int bool;
#define true 1
#define false 0


uint32_t MAX_DATA_AT_ONCE;
uint32_t FILE_SIZE_IN_BYTES;
uint32_t NUMPACKETS;

void validateCommandLineArguments(int argc, char * argv[]);
void validateCommandLineArguments(int argc, char * argv[]) {
	if (argc != 5) {
		printf(
			"Check number of arguments (expected 5 , got %d) - Usage %s hostname portnumber secretkey filename configfile.dat\n",
			argc, argv[0]);
		exit(1);
	}

	validateSecretKey(argv[3]);
	validateFileName(argv[4]);
	validateConfigFile(argv[5]);
}

void validateFileName(char * filename);
void validateFileName(char * filename) {
	const char * invalid_filename_characters = " \n\t//";
	char *string_iter = filename;
	size_t filename_len = strlen(filename);
	if (filename_len <= 16 && filename_len >= 0) {
		while (*string_iter) {
			if (strchr(invalid_filename_characters, *string_iter)) {

				perror("Filename cannot contain spaces or forward slashes ('/'), and it must not exceed 16 ASCII characters.");
				exit(1);
			}

			string_iter++;
		}
	} else {
		perror(
			"Filename cannot contain spaces or forward slashes ('/'), and it must not exceed 16 ASCII characters.");
		exit(1);

	}

}

void validateSecretKey(char *secretKey);
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

void validateConfigFile(char * filename);
void validateConfigFile(char * filename)
{

	FILE * fp;
	fp = fopen (filename, "r");
	rewind(fp);
	fscanf(fp, "%d", &MAX_DATA_AT_ONCE);
	fclose(fp);
}


int main(int argc, char * argv[]) {

	// Validate the command line arguments
	validateCommandLineArguments(argc, argv);
	struct FilePacket
	{
		char filedata[MAXDATA];

	} filePackets[NUMPACKETS];

	// Create the sending buffer and the receiving buffer
	char recvBuffer[BUFSIZ];
	char sendBuffer[BUFSIZ];

	// Setup the socket
	struct addrinfo addressInfo;
	memset(&addressInfo, 0, sizeof addressInfo);

	addressInfo.ai_family = AF_INET; // Use IPv4
	addressInfo.ai_socktype = SOCK_STREAM; // Use TCP

	//  populate the address info
	struct addrinfo *availableServerSockets;
	int getAddrCheck;
	if ((getAddrCheck = getaddrinfo(argv[1], argv[2], &addressInfo,
			&availableServerSockets)) != 0) { //If it fails...
		printf("Failed to get address info for %s failed ", argv[1]);
	return -1;
}

int sockfd;
bool connectionSuccessful = false;
while (availableServerSockets != NULL) {
	bool error = false;

	if ((sockfd = socket(availableServerSockets->ai_family,
		availableServerSockets->ai_socktype,
				availableServerSockets->ai_protocol)) == -1) { //If it fails...
		error = true;
}

if (connect(sockfd, availableServerSockets->ai_addr,
	availableServerSockets->ai_addrlen) == -1) {
	error = true;
}
if (error)
	availableServerSockets = availableServerSockets->ai_next;
else {
	connectionSuccessful = true;
	break;
}
}

if (!connectionSuccessful) {
	printf("Could not connect to host \n");
	return 2;
} else {
		// Successfully connected to server
}
freeaddrinfo(availableServerSockets);

	// Construct message
char str[BUFSIZ];
sprintf(str, "$%s$", argv[3]);
strcat(str, argv[4]);

	// Write the request
snprintf(sendBuffer, sizeof(sendBuffer), "%s", str);
write(sockfd, sendBuffer, strlen(sendBuffer));

	// Read the response
int numReadBytes;
while ((numReadBytes = read(sockfd, recvBuffer, sizeof(recvBuffer) - 1)) > 0) {
	recvBuffer[numReadBytes] = '\0';
	printf("%s\n", recvBuffer);
}

if (numReadBytes < 0) {
	printf("\n Error reading \n");
}
close(sockfd);
return 0;
}
