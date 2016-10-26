#define _DEFAULT_SOURCE

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
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <time.h>

typedef int bool;
#define true 1
#define false 0



uint32_t BLOCKSIZE = -1;

void validateCommandLineArguments(int argc, char * argv[]);
void validateFileName(char * filename);
void validateSecretKey(char *secretKey);
void validateConfigFile(char * configFilename);
bool validateCheckSum(char * filename);

long long int RTT(struct timeval *t1 , struct timeval *t2);

// Calculate the round trip time given two timevals
// to a granularity of milliseconds
long long int RTT(struct timeval *t1 , struct timeval *t2)
{
	struct timeval tv1 = *t1;
	struct timeval tv2 = *t2;
	long long int milliseconds;
	milliseconds = (long long int) (tv2.tv_usec - tv1.tv_usec) / 1000;
	milliseconds += (long long int) (tv2.tv_sec - tv1.tv_sec) *1000;
	return milliseconds;
}
// Overall validation of command line arguments
void validateCommandLineArguments(int argc, char * argv[]) {
	if (argc != 6) {
		printf(
			"Check number of arguments (expected 6 , got %d) - Usage %s hostname portnumber secretkey filename configfile.dat\n",
			argc, argv[0]);
		exit(1);
	}

	validateSecretKey(argv[3]);
	validateFileName(argv[4]);
	validateConfigFile(argv[5]);
}

// Validate that the filename is of specified length and doesn't contain illegal characters
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


	// So far, the filename is correct.
	// Now, check if file already exists

	// Make the file path	
	char const * dir = "";
	char filePath[ 80 ] = { 0 };
	char gcwd[1024];
	if (getcwd(gcwd, sizeof(gcwd)) == NULL)
		perror("getcwd() error");
	strcat(filePath, gcwd);
	strcat(filePath, "/");
	strcat(filePath, filename);
	// Next, call access to check if the filepath is legal. If file exists balk. Else, its a new file download
	// so continue onwards.
	printf("File name %s \n ", filePath);
	if( access( filePath, F_OK ) == -1 ) {
		return;
	} else {
		printf("-- File already exists! Delete and retry-- \n");
		exit(EXIT_FAILURE);
	}

}
// Validating the secret key is of legal size and is comprised of ascii characters solely.
void validateSecretKey(char * secretKey)
{
	size_t secretKey_len = strlen(secretKey);

	if(secretKey_len <10 || secretKey_len > 20 )
	{
		perror("Secret Key must be at least length 10 but not more than 20");
		exit(1);
	}
	int i;
	for (i=0; i <= secretKey_len;i++)
	{
		if (isascii(secretKey[i]) == 0 )
		{
			perror("Secret Key must be ASCII representable.");
			exit(EXIT_FAILURE);
		}
	}
}
// Validating that the configuration file is available for use
// And that a number is read from the file to designate the block size
void validateConfigFile(char * configFilename)
{
	if( access( configFilename, F_OK ) != -1 ) {
		FILE * fp;
		fp = fopen (configFilename, "r");
		rewind(fp);
		fscanf(fp, "%d", &BLOCKSIZE);
		if(BLOCKSIZE == -1)
		{
			perror("Blocksize is -1 which is invalid. Ensure ConfigFile.dat has a number >0.");
			exit(EXIT_FAILURE);
		}
		fclose(fp);
	}
	else
	{
		perror("Configfile.dat does not exist in filesystem.");
		exit(EXIT_FAILURE);
	}
}

// This is the same code as myunchecksum from lab1. It checks if the checksum attached to the last 8 bytes
// of the file received from the server matches the computed checksum generated as part of summing the individual bytes of the file
// If this value matches then we continue forth , else we will have to abort the program.
int validateCheckSum(char * filename)
{
	char c;
	unsigned long long check_sum =0;
	int fd, fdw;
	fd = open(filename, O_RDONLY);
	if (fd == -1) {
		printf("Error opening file\n");
		return;
	}
	int counter = 0;
	char buffer[1];
	ssize_t nrd = -1;

	// Grab the last 8 bytes of the file read.
	char last8[8];
	while ((nrd = read(fd, buffer, 1)))
	{
		check_sum += buffer[0];
	}

	int read_byte;
	lseek(fd, -8L, SEEK_END);
	read_byte = read(fd, last8, 8);

	// remove from check_sum the last 8 bytes of the file downloaded
	int a;
	for(a = 0 ; a < 8 ; a++)
	{
		check_sum -= last8[a];

	}
	uint64_t small_check_sum = (*(uint64_t *) last8);
	unsigned long long lhs = (unsigned long long ) be64toh(small_check_sum);
	unsigned long long rhs = check_sum;
	int verdict = 0;
	// Perform the match on the expected checksum and given checksum
	if(lhs == rhs)
	{
		// The verdict is 1 meaning it was a match
		verdict = 1;
	}
	else
	{
		printf("Checksum Verdict: %s\n","does not match");
		verdict = 0;
	}

	close(fd);
	return verdict;
}



int main(int argc, char * argv[]) {

		// Validate the command line arguments
	validateCommandLineArguments(argc, argv);

	// This the startwatch, stopwatch timevals
	struct timeval startWatch;
	struct timeval stopWatch;
	

	// Create the sending buffer and the receiving buffer
	// Here the recvbuffer must have size BLOCKSIZE as designated by the value in the configfile.dat
	char recvBuffer[BLOCKSIZE];
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
			printf("Error: Failed to get address info for %s failed \n", argv[1]);
		exit(EXIT_FAILURE);
	}
	// Create the full association with the server socket.
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
	exit(EXIT_FAILURE);
} else {
			// Successfully connected to server
}

// No longer need the available client sockets we can create full association with
freeaddrinfo(availableServerSockets);

// Construct message
char str[BUFSIZ];
sprintf(str, "$%s$", argv[3]);
strcat(str, argv[4]);

// Write the request
snprintf(sendBuffer, sizeof(sendBuffer), "%s", str);
write(sockfd, sendBuffer, strlen(sendBuffer));

// Shut it down so the no more writes need can be done on the socket.
// Reads can still be done.
shutdown(sockfd, SHUT_WR);

// File descriptor for the file we are about to download and store.
int fdw;

	// Read the response
memset(&recvBuffer[0], 0, BLOCKSIZE);
int numReadBytes = -1;
int totalNumReadBytes = 0;


//First Read
{
	numReadBytes = read(sockfd, recvBuffer, BLOCKSIZE);
	char * filename = argv[4];
	if(numReadBytes > 0)
	{
		// Open the file if not exists. If it already exists we couln't have come so far because of validateFileName called
		// in the commandLinArguments validation code.
		fdw = open(filename, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
		if(fdw == -1)
		{
			perror("Error: In creating new file.");
    		close(sockfd);
    		exit(EXIT_FAILURE);
		}

	}
	else
	{
		// No file was found on the server 
		// So can't download exit with failure.
		printf("Error : No such file on server. \n");
		close(sockfd);
		exit(EXIT_FAILURE);

	}
	// If the file was found start writing the bytes read into the file.
	write(fdw,recvBuffer,numReadBytes);
	memset(&recvBuffer[0], 0, BLOCKSIZE);
	totalNumReadBytes += numReadBytes;
	if (-1 == gettimeofday(&startWatch, NULL)) {
		perror("resettimeofday: gettimeofday");
		close(sockfd);
		close(fdw);
		return EXIT_FAILURE;
	}
	// If no bytes were read then error reading
	if (numReadBytes < 0) {
		printf("\n Error reading \n");
		close(sockfd);
		close(fdw);
		return EXIT_FAILURE;
	}
}

//Subsequent Reads
{
	while ((numReadBytes = read(sockfd, recvBuffer, BLOCKSIZE)) > 0) {
		write(fdw,recvBuffer,numReadBytes);
		memset(&recvBuffer[0], 0, BLOCKSIZE);
		totalNumReadBytes += numReadBytes;
	}

	if (numReadBytes < 0) {
		printf("\nError : In reading \n");
		close(sockfd);
		close(fdw);
		return EXIT_FAILURE;
	}
}

//Last Read Attempted
{
	if (-1 == gettimeofday(&stopWatch, NULL)) {
		perror("resettimeofday: gettimeofday");
		close(sockfd);
		close(fdw);
		return EXIT_FAILURE;
	}
}
// Validate that the checksum was correct. If not exit the program.
if(validateCheckSum(argv[4]) == 1)
{
	long long int time_taken = RTT(&startWatch, &stopWatch);
	double completion = time_taken/1000.0;
	printf("<- <- <- Generated Report -> -> -> \n ");
	printf("Number of bytes transfered : %d Bytes\n", totalNumReadBytes);
	printf("Completion Time : %lf seconds \n", completion);
	printf("Reliable Throughput : %lf MBps \n" , (totalNumReadBytes/(1000000.0*completion)));
}
else
{
	close(sockfd);
	close(fdw);
	return EXIT_FAILURE;

}
close(sockfd);
close(fdw);
return EXIT_SUCCESS;
}
