#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/time.h>
#include <libgen.h>
#include <stdbool.h>
#include <netdb.h>

FILE *f;

#define DEBUG 0

#ifdef DEBUG
#define DEBUG_PRINT(...) do{ fprintf( f, __VA_ARGS__ ); } while( false )
#else
#define DEBUG_PRINT(...) do{ } while ( false )
#endif
uint32_t BLOCKSIZE = -1;

int start_turbo_server();
void validateCommandLineArguments(int argc, char * argv[]);
void validateFileName(char * filename);
void validateSecretKey(char *secretKey);
void validateConfigFile(char * configFilename);
bool validateCheckSum(char * filename);
int create_socket_to_listen_on(char *rand_port);
size_t send_socket_data(int in_fd, char * message, int size, struct sockaddr * server_addr);
size_t recv_socket_data(int in_fd, char * buffer, int size);
long getTimeDifference(struct timeval *t1 , struct timeval *t2);
long long int RTT(struct timeval *t1 , struct timeval *t2);

struct sockaddr_in * server_to_transact_with;

int portno;
char *map;
int fd;
int result;
long int sendSize;
int currSeq=0;
struct stat finfo;
int numPackets;

void error(const char *msg)
{
    perror(msg);
    exit(0);
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
    //printf("File name %s \n ", filePath);
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
bool validateCheckSum(char * filename)
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
    return verdict == 1 ? true : false;
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
    if (sd < 0) error("Opening socket");
    int sockfd = sd;
    int flags = 0;
    //parameters for select
    struct timespec tv;
    fd_set readfds;
    //set the timeout values
    tv.tv_sec = 0;
    tv.tv_nsec = 100000000;
    //sockfd has to be monitored for timeout
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);
    return sd;
}

size_t send_socket_data(int in_fd, char * message, int size, struct sockaddr * server_addr)
{
    size_t numSent;
    struct sockaddr recv_sock;
    socklen_t addr_len = sizeof(recv_sock);
    struct sockaddr * who_to_send_addr = server_addr;
    numSent = sendto(in_fd, message, size , 0, who_to_send_addr, addr_len);
    return numSent;
}
size_t recv_socket_data(int in_fd, char * buffer, int size)
{
    struct sockaddr_in from;
    socklen_t length=sizeof(struct sockaddr_in);
    int n = recvfrom(in_fd,buffer,size ,0,(struct sockaddr *)&from, &length);
    if (n < 0) error("recvfrom");
    return n;
}

long getTimeDifference(struct timeval *t1 , struct timeval *t2)
{
    struct timeval tv1 = *t1;
    struct timeval tv2 = *t2;
    long milliseconds;
    milliseconds = (tv2.tv_usec - tv1.tv_usec) / 1000;
    milliseconds += (tv2.tv_sec - tv1.tv_sec) *1000;
    return milliseconds;
}

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


int start_turbo_client (int argc, char **argv) {

    // This the startwatch, stopwatch timevals
    struct timeval startWatch;
    struct timeval stopWatch;
    //udp setup
    int length;
    socklen_t fromlen;
    struct sockaddr_in server;
    struct sockaddr_in from;
    char buf[1024];
    int sockfd = create_socket_to_listen_on(argv[1]);
    // Construct message
    char str[BUFSIZ];
    sprintf(str, "$%s$", argv[3]);
    strcat(str, argv[4]);
    char sendBuffer[BUFSIZ];
    // Write the request
    snprintf(sendBuffer, sizeof(sendBuffer), "%s", str);

    
    server.sin_family = AF_INET;
    struct hostent *hp, *gethostbyname();
    hp = gethostbyname(argv[1]);
    bcopy ( hp->h_addr, &(server.sin_addr.s_addr), hp->h_length);
    server.sin_port = htons(atoi(argv[2]));
    server_to_transact_with = &server;
     /* connect: create a connection with the server */
    if (connect(sockfd,(struct sockaddr *)&server, sizeof(server)) < 0) 
        error("ERROR connecting");

    if(send_socket_data(sockfd, sendBuffer, strlen(sendBuffer), (struct sockaddr *) &server) <0)
        error("Sending handshake connection set up");

    // File descriptor for the file we are about to download and store.
    int fdw;

    int numReadBytes = -1;
    int totalNumReadBytes = 0;

    char buffer[256];
    int n = read(sockfd,buffer,255);
    if (n < 0)
        error("ERROR reading from socket");
    char ack[] = "ACK";
    send_socket_data(sockfd, ack, strlen(ack), (struct sockaddr *) server_to_transact_with);

    if (-1 == gettimeofday(&startWatch, NULL)) {
        perror("resettimeofday: gettimeofday");
        close(sockfd);
        close(fdw);
        return EXIT_FAILURE;
    }

    long int fileSize=atoi(buffer);
    //open file for writing
    char *map;
    int fd;
    int result;
    char * fileName = argv[4];
    fd = open(fileName, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
    if (fd == -1) {
        error("open");
    }
    result = lseek(fd, fileSize-1, SEEK_SET);
    if (result == -1) {
        close(fd);
        error("lseek");
    }
    
    result = write(fd, "", 1);
    if (result != 1) {
        close(fd);
        error("write");
    }
    map = (char*)mmap(0, fileSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
        close(fd);
        error("map");
    }
    //finished mmap setup
    
    char recvData[BLOCKSIZE];
    char data[BLOCKSIZE];
    long int recvSize=0;
    long int totPkts=((fileSize + BLOCKSIZE -1)/BLOCKSIZE);
    printf("Total packets expected %ld \n", totPkts);
    char pktsRcvd[1000000]={};
    int nackCounter=0;

    fromlen = sizeof(struct sockaddr_in);
    
    //parameters for select
    struct timespec tv;
    fd_set readfds;
    //set the timeout values
    tv.tv_sec = 0;
    tv.tv_nsec = 100000000;
    //sockfd has to be monitored for timeout
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);
    
    while(1) {

        bzero(recvData, BLOCKSIZE + 8);
        bzero(data, BLOCKSIZE);
        //wait fro recvfrom or timeout
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        //printf("On top of select\n");
        int setRet = pselect(sockfd+1, &readfds, NULL, NULL, &tv, NULL);
        
        //if (FD_ISSET(sockUDP, &readfds)) {
        if(setRet > 0 ){
        n = recvfrom(sockfd,recvData,BLOCKSIZE + 8,0,(struct sockaddr *)&from, (socklen_t*)&length);
        if (n < 0) error("recvfrom");
        char seqC[8];
        memcpy(seqC,recvData,8);
        int seq=atoi(seqC);
        if(pktsRcvd[seq]==0){
            //printf("Sequence Received %d\n",seq);
            numPackets++;
            pktsRcvd[seq]=1;
            char * pch = recvData;
            while(pch!=NULL)
            {
                if(*pch == '|') break;
                pch = pch+1;
            }
            memcpy(data,pch+1,BLOCKSIZE);
            DEBUG_PRINT("\n What i received was %s \n", recvData);
                //printf("seq num: %d\n", seq);

                //fseek(f,seq*1024,SEEK_SET);
                //fwrite(data,sizeof(data),1,f);
            if (seq < totPkts-1){
              //  printf("Received size is %ld\n", recvSize );
                memcpy(&map[seq*BLOCKSIZE],data,sizeof(data));
                recvSize+=sizeof(data);
            }
            else{
                //printf("Received size is %ld\n", recvSize );
                //printf("Writing last packet\n");
                memcpy(&map[seq*BLOCKSIZE],data,fileSize-((seq)*BLOCKSIZE));
                recvSize+=(fileSize-((seq)*BLOCKSIZE));
            }
            //printf("Received size is | file size is %ld %ld \n", recvSize, fileSize);
            //printf("Number of bytes: %ld\n",sizeof(data));
            if(recvSize>=fileSize)
            {
                printf("File has been received :) \n");
                int final[1];
                final[0] = -1;
                n = sendto(sockfd,final,sizeof(final),0,(struct sockaddr *)&from, length);
                if (n < 0) error("ERROR sending final packet");
                    n = sendto(sockfd,final,sizeof(final),0,(struct sockaddr *)&from, length);
                if (n < 0) error("ERROR sending final packet");
                    n = sendto(sockfd,final,sizeof(final),0,(struct sockaddr *)&from, length);
                if (n < 0) error("ERROR sending final packet");

                if (-1 == gettimeofday(&stopWatch, NULL)) {
                    perror("resettimeofday: gettimeofday");
                    goto EXIT_FAILURE_CODE;
                }
                long long int time_taken = RTT(&startWatch, &stopWatch);
                double completion = time_taken/1000.0;
                printf("<- <- <- Generated Report -> -> -> \n ");
                printf("Completion Time : %lf seconds \n", completion);
                printf("Reliable Throughput : %lf MBps \n" , (totalNumReadBytes/(1000000.0*completion)));
                printf("File received\n");
                printf("Filesize: %ld\n", fileSize);
                printf("Recv size: %ld\n", recvSize);
                    //printf("Total Nack packets: %d\n",nackCounter);

                printf("Total Nack packets: %d\n",nackCounter);
                    //fclose(f);
                if (munmap(map, fileSize) == -1) {
                    error("munmap");

                }
                close(fd);
                break;
                }
            }
        }
        else {
                //printf("Number of bytes rxed: %ld\n",recvSize);
                //printf("Entering timeout loop");
                int control[BLOCKSIZE];
                memset(control, -2, BLOCKSIZE);
                long int reqs=0;
                int j;
                int num=0;
                //bzero(control,512);
                for(j=0;j<totPkts;j++) {
                    if(pktsRcvd[j]==0) {
                        //printf("Sending NACK:%d\n",j);
                        control[num] = j;
                        num++;
                        reqs++;
                        if(reqs==BLOCKSIZE) {
                            nackCounter++;
                            num=0;
                            n = sendto(sockfd,control,sizeof(control),0,(struct sockaddr *)&from, length);
                            if (n < 0) error("NACK sendto error");

                            n = sendto(sockfd,control,sizeof(control),0,(struct sockaddr *)&from, length);
                            if (n < 0) error("NACK sendto error");

                            break;

                        }
                    }
                }
                if(j==totPkts)
                {
                    if(num>0)
                    {
                        nackCounter++;
                        n = sendto(sockfd,control,sizeof(control),0,(struct sockaddr *)&from, length);
                        if (n < 0) error("NACK sendto error");
                
                        n = sendto(sockfd,control,sizeof(control),0,(struct sockaddr *)&from, length);
                        if (n < 0) error("NACK sendto error");
                    }
                
                }
                // printf("Number of bytes rxed: %ld\n",recvSize);
                FD_ZERO(&readfds);
                FD_SET(sockfd, &readfds);

            }
        }
    //udp
    EXIT_SUCCESS_CODE: close(sockfd);
                   return EXIT_SUCCESS;
    EXIT_FAILURE_CODE: close(sockfd);
                return EXIT_FAILURE;
}

int main(int argc, char *argv[])
{
    f =  fopen("turboclient.txt", "w+");
    if (f == NULL)
    {
        printf("Error opening file!\n");
        exit(1);
    }
    validateCommandLineArguments(argc, argv);
    int result = start_turbo_client(argc,argv);
    fclose(f);
    return result;
}