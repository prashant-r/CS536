import java.io.*;
import java.util.*;


#define MAXIMUM_WAITING_CLIENTS 10
#define MAX_BUF 1024


uint32_t BLOCKSIZE;
uint32_t FILE_SIZE_IN_BYTES;
uint32_t NUMPACKETS;

void handle_sigchld(int sig);
void validateCommandLineArguments(int argc, char * argv[]);
int setUpHalfAssociation(char * PORT);
void validateConfigFile(char * filename);
void validateSecretKey(char * secretKey);
bool validateFile(char * filename);


void handle_sigchld(int sig) {
	int saved_errno = errno;
	while (waitpid((pid_t)(-1), 0, WNOHANG|WUNTRACED) > 0) {}
       errno = saved_errno;
}

// Sets up the half association 

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
		exit(EXIT_FAILURE);
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
// Validates the command line arguments
void validateCommandLineArguments(int argc, char * argv[])
{
	if(argc != 4)
	{
		printf("Check number of arguments (expected 3 , got %d) - Usage portnumber secretkey configfile.dat\n", argc);
		exit(1);
	}
	validateSecretKey(argv[2]);
	validateConfigFile(argv[3]);
}
// Validates the config file exists and read the number in the file.
void validateConfigFile(char * filename)
{

	FILE * fp;
	fp = fopen (filename, "r");
	rewind(fp);
	fscanf(fp, "%d", &BLOCKSIZE);
	fclose(fp);
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
			exit(1);
		}
	}
}
// Validate that the filename is of specified length and doesn't contain illegal characters
bool validateFile(char * filename)
{
	const char * invalid_filename_characters = " \n\t//";
	char *string_iter = filename;
	size_t filename_len = strlen(filename);
	if (filename_len <= 16 && filename_len >= 0) {
		while (*string_iter) {
			if (strchr(invalid_filename_characters, *string_iter)) {

				perror("Filename cannot contain spaces or forward slashes ('/'), and it must not exceed 16 ASCII characters.");
				return false;
			}

			string_iter++;
		}
	} else {
		perror(
			"Filename cannot contain spaces or forward slashes ('/'), and it must not exceed 16 ASCII characters.");
		return false;

	}

	/*

		Check if the file exists in the file deposit directory. 


	*/
	// Make the file path	
	char const * dir = "filedeposit";
    char filePath[ 80 ] = { 0 };
    strcat(filePath, dir );
    strcat(filePath, "/");
    strcat(filePath, filename);
    // if the file exists then return true else false.

    if( access( filePath, F_OK ) != -1 ) {
    	return true;
    } else {
    	printf("File doesn't exist \n");
    	return false;
    }
}

int main(int argc, char * argv[])
{

	//NUMPACKETS = (FILE_SIZE_IN_BYTES + BLOCKSIZE - 1) / BLOCKSIZE;
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

		// At this point constructed the full association
		fullAssocSockfd = accept(halfAssocSockfd, (struct sockaddr *)&clientAddress, &sin_size);

		if(fullAssocSockfd == -1)
		{
			close(fullAssocSockfd);
			close(halfAssocSockfd);
			exit(EXIT_FAILURE);
		}
		int child;
		// Fork a child process to carry out actual file transfer
		if (!(child =fork()))
		{

			// this is the child process
			close(halfAssocSockfd);

            char requestBuf[BUFSIZ];
            char buf[BLOCKSIZE];

			// Read the response
            int numReadBytes;
            if ((numReadBytes = read(fullAssocSockfd, requestBuf, sizeof(requestBuf))) > 0)
            {
                requestBuf[numReadBytes] = '\0';
            }

			// Decode the message
            char *saveptr;
            char *secretKey, *filename;
            secretKey = strtok_r(requestBuf, "$", &saveptr);
            filename = strtok_r(NULL, "$", &saveptr);

			// file descriptor
            int fd;
            // If the secret keys match then continue onwards
            if(strcmp(secretKey, secretKeyGiven ) == 0)
            {
                if(validateFile(filename) == true)
                {
                	// Check if the filename provided by client is valid 
                	// and exists in the filedeposit directory.

                    unsigned long long check_sum =0;
                    char const * dir = "filedeposit";
                    char filePath[ 80 ] = { 0 };
                    strcat(filePath, dir );
                    strcat(filePath, "/");
                    strcat(filePath, filename);

                    if ((fd = open(filePath,O_RDONLY))==-1){
                      perror("failed to open file");
                      return EXIT_FAILURE;
                  }
                  // Send the packets one by one of size BLOCKSIZE
                  // Compute the checksum of the file and add it to the end of the file later
                  
                  int num_read = -1;
                  while((num_read = read(fd,buf,BLOCKSIZE)) > 0){

                     int x;
                     for(x =0 ; x < num_read; x++)
                     {
                        check_sum+= buf[x];
                    }
                    if(num_read==-1){
                     perror("failed to read file \n ");
                     return EXIT_FAILURE;
                 }
                 // Write out the characters
                 int num_written = write(fullAssocSockfd, buf, num_read);
                 if(num_written==-1){
                     perror("failed to write file to fullAssocSockfd \n");
                     return EXIT_FAILURE;
                 }
                 memset(&buf[0], 0, BLOCKSIZE);
             }
             // convert checksum to big endian as per lab 1
             check_sum = htobe64(check_sum);
             // write out the check sum 
             write(fullAssocSockfd, (void *)&check_sum, 8);
             // write out the 0 byte so as to signify end of transmission.
             write(fullAssocSockfd, buf, 0);
             close(fullAssocSockfd);
             exit(EXIT_SUCCESS);
         }
         else
         {

			// Kill Gracefully, Close the connection with failed error. Ignore the request bad filename specified by client.
            close(fullAssocSockfd);
            exit(EXIT_FAILURE);
        }    
    }
    else
    {
        printf("Secret Key Mismatch Given Key : %s | Expected Key : %s \n", secretKeyGiven, secretKey);
    }
    exit(EXIT_SUCCESS);
}
close(fullAssocSockfd);

}
close(halfAssocSockfd);
return EXIT_SUCCESS;
}




