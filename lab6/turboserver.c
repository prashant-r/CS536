#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <netdb.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/time.h>
#include <stdbool.h>

#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))

FILE *f;
#define DEBUG 0

#ifdef DEBUG
#define DEBUG_PRINT(...) do{ fprintf(f, __VA_ARGS__ ); } while( false )
#else
#define DEBUG_PRINT(...) do{ } while ( false )
#endif


int LOSS_NUM;
int portno;
char *map;
int fd;
int result;
long int sendSize;
int currSeq=0;
struct stat finfo;
int numPackets=0;
uint32_t BLOCKSIZE = -1;
void validateConfigFile(char * filename);
void validateSecretKey(char * secretKey);
bool validateFile(char * filename);
size_t dropsendto(int in_fd, char * message,int size, struct sockaddr * server_addr, int totalnum, int lossnum);
bool nackhandle (int sockUDP, struct sockaddr_in from, socklen_t fromlen);
void error(const char *msg)
{
    perror(msg);
    exit(0);
}
int packet_counter = 1;
int loss_counter = 1;
bool nackhandle (int sockUDP, struct sockaddr_in from, socklen_t fromlen) {
    char buffer[256];
    int n,i;
    bzero(buffer,256);
    int control[BLOCKSIZE];
    while(1) {
        bzero(control,BLOCKSIZE);
        /*n = read(sock,control,sizeof(control));
         if (n < 0) error("ERROR reading from socket");*/
        n = recvfrom(sockUDP,control,sizeof(control),0,(struct sockaddr *)&from,&fromlen);
        if (n < 0) error("ERROR in recieving NACK");
        if (control[0] == -1) {
            printf("File Sent\n");
            return true;
        }
        else {

            char seqC[8];
            char data[BLOCKSIZE];
            char sendData[BLOCKSIZE + 8];
            char seqChar[9];
            for(i=0;i<BLOCKSIZE;i++){
                //printf("control [i] is %d \n", control[i]);
                bzero(data, BLOCKSIZE);
                bzero(sendData, BLOCKSIZE);
                if(control[i] <0)
                {
                    break;
                }
                if (control[i] < numPackets-1){
                    memcpy(data,&map[control[i]*BLOCKSIZE],BLOCKSIZE);
                    sprintf(seqChar,"%7d|",control[i]);
                    memcpy(sendData,seqChar,8);
                    memcpy(sendData+8,data,BLOCKSIZE);
                    n = dropsendto(sockUDP,sendData,sizeof(sendData),(struct sockaddr *)&from,1000,LOSS_NUM);
                    if (n  < 0) error("sendto");
                }
                else{
                    if(MAX(0,finfo.st_size-(control[i]*BLOCKSIZE)) == 0)
                    {
                        char dat[1];
                        dat[0] = 'A'; 
                        n = dropsendto(sockUDP,dat,0,(struct sockaddr *)&from,10,9);
                        if (n  < 0) error("sendto");
                    }
                    else{
                    memcpy(data,&map[control[i]*BLOCKSIZE],MAX(0,finfo.st_size-control[i]*BLOCKSIZE));
                    sprintf(seqChar,"%7d|",control[i]);
                    memcpy(sendData,seqChar,8);
                    memcpy(sendData+8,data,BLOCKSIZE);
                    n = dropsendto(sockUDP,sendData,sizeof(sendData),(struct sockaddr *)&from,1000,LOSS_NUM);
                    if (n  < 0) error("sendto");              
                    }
                }
            }
            return false;
        }
    }
}


// Validates the command line arguments
void validateCommandLineArguments(int argc, char * argv[])
{
    if(argc != 5)
    {
        printf("Check number of arguments (expected 3 , got %d) - Usage portnumber secretkey configfile.dat lossnum\n", argc);
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
    char const * dir = "/tmp/";
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
    return sd;
}
struct sockaddr * client_to_transact_with;

int main(int argc, char *argv[]) {
    f =  fopen("turboclient.txt", "w+");
    if (f == NULL)
    {
        printf("Error opening file!\n");
        exit(1);
    }
    validateCommandLineArguments(argc, argv);
    char * secretKeyGiven = argv[2];
    int sockfd = create_socket_to_listen_on(argv[1]);
    LOSS_NUM = atoi(argv[4]);
    char requestBuf[BUFSIZ];
    char buf[BLOCKSIZE];

    struct sockaddr_in from;
    socklen_t length=sizeof(struct sockaddr_in);
    // Read the response
    int numReadBytes;
    if ((numReadBytes = recvfrom(sockfd,requestBuf,sizeof(requestBuf),0,(struct sockaddr *)&from, &length)) > 0)
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

            char const * dir = "/tmp";
            char filePath[ 80 ] = { 0 };
            strcat(filePath, dir );
            strcat(filePath, "/");
            strcat(filePath, filename);
            if ((fd = open(filePath,O_RDONLY))==-1){
              perror("failed to open file");
              return EXIT_FAILURE;
            }
            char buffer[256];
            //get file details
            if (-1 == stat(filePath, &finfo)) {
              error("error stating file!\n");
              exit(0);
            }
            bzero(buffer,256);
            sprintf(buffer, "%lld", (long long) finfo.st_size);
            //send file details
            client_to_transact_with = (struct sockaddr *)&from;

            unsigned long long int numbytes;
            // loop for initial SYN/ACK 
            while(1) {
                if ((numbytes = dropsendto(sockfd,buffer,sizeof buffer,client_to_transact_with, 1000, LOSS_NUM)) == -1) {
                perror("sender: sendto");
                return;
                }
            
                char ack_message[4];
                int response_len = recv(sockfd, &ack_message, sizeof(ack_message), 0);
                if (response_len >= 0) {
                    ack_message[3] = '\0';
                    //printf("sender: got %s\n", ack_message);
                    break;
                }
                else if (response_len == -1) {
                    perror("sender: receive ACK");
                }
            }

            int n;
            //udp
            int sockUDP = sockfd;
            unsigned int length;
            struct sockaddr_in server, from;
            struct hostent *hp;
            int nackCounter = 0;
            bool isFileSent=false;
            socklen_t fromlen;
            fromlen = sizeof(struct sockaddr_in);

       
            map = (char*)mmap(0, finfo.st_size, PROT_READ, MAP_SHARED, fd, 0);
            if (map == MAP_FAILED) {
              close(fd);
              error("map");
          }    
          numPackets=((finfo.st_size + BLOCKSIZE -1)/BLOCKSIZE);

         
          while (1) 
          {

            long int size=finfo.st_size;
            //long int totPkts=(size/1024)+1;
            long int i;
            for(i=0;i<size;i++) {
                char data[BLOCKSIZE];
                char sendData[BLOCKSIZE + 8];
                char seqChar[9];
                if(currSeq == numPackets+1)
                {
                    isFileSent=true;
                    break;
                }

                if (currSeq < numPackets){ //all packets except the last packet
                    sprintf(seqChar,"%7d|",currSeq);
                    memcpy(sendData,seqChar,8);
                    memcpy(sendData+8,&map[currSeq*BLOCKSIZE],BLOCKSIZE);
                    n = dropsendto(sockUDP,sendData,sizeof(sendData),client_to_transact_with,1000,LOSS_NUM);
                    if (n  < 0) error("sendto");
                    currSeq++;
                }
                else{
                    if(MAX(0, finfo.st_size-(currSeq*BLOCKSIZE)) == 0)
                    {
                        n = dropsendto(sockUDP,sendData,sizeof(sendData),client_to_transact_with,1000,LOSS_NUM);
                        if (n  < 0) error("sendto last seq");
                        currSeq++;
                        
                    } 
                    else
                    {
                    sprintf(seqChar,"%7d|",currSeq);
                    memcpy(sendData,seqChar,8);
                    memcpy(sendData+8,&map[currSeq*BLOCKSIZE],MAX(0, finfo.st_size-(currSeq*BLOCKSIZE)));
                    n = dropsendto(sockUDP,sendData,sizeof(sendData),client_to_transact_with,1000,LOSS_NUM);
                    if (n  < 0) error("sendto last seq");
                    currSeq++;
                    }
                }
            }
            if(isFileSent)
            {
                if(nackhandle(sockUDP, server, fromlen))
                    break;
            }
            }
            close(sockfd);
            exit(EXIT_SUCCESS);
        }
    }
    else
    {
        printf("Secret Key Mismatch Given Key : %s | Expected Key : %s \n", secretKeyGiven, secretKey);
    }
    close(sockfd);
    fclose(f);
    exit(EXIT_SUCCESS);

}

size_t dropsendto(int in_fd, char * message,int size, struct sockaddr * server_addr, int totalnum, int lossnum)
{
    if(packet_counter <totalnum)
    {
        packet_counter++;
    }
    else if(packet_counter == totalnum)
    {
        if(loss_counter == lossnum)
        {
            packet_counter = 1;
            loss_counter = 1;
            return 0;
        }
        else{
            loss_counter ++;
            return 0;
        }
    }
    size_t numSent;
    struct sockaddr recv_sock;
    socklen_t addr_len = sizeof(recv_sock);
    struct sockaddr * who_to_send_addr = server_addr;
    DEBUG_PRINT("\n -------------- The message sent was-------------------- \n %s", message);
    numSent = sendto(in_fd, message, size , 0, who_to_send_addr, addr_len);
    return numSent;
}