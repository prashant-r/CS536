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
    printf("Entering UDP listner\n");
    while(1) {
        printf("Waiting for UDP NACK...\n");
        bzero(control,BLOCKSIZE);
        /*n = read(sock,control,sizeof(control));
         if (n < 0) error("ERROR reading from socket");*/
        n = recvfrom(sockUDP,control,sizeof(control),0,(struct sockaddr *)&from,&fromlen);
        if (n < 0) error("ERROR in recieving NACK");
        printf("NACK: %d\n",control[0]);
        printf("NACK: \n");
        if (control[0] == -1) {
            printf("File Sent\n");
            return true;
        }
        else {

            char seqC[8];
            char data[BLOCKSIZE];
            char sendData[BLOCKSIZE];
            char seqChar[9];
            for(i=0;i<BLOCKSIZE;i++){
                //printf("control [i] is %d \n", control[i]);
                if(control[i] <0)
                {
                    break;
                }
                if (control[i] < numPackets-1){
                    memcpy(data,&map[control[i]*BLOCKSIZE],BLOCKSIZE);
                    sprintf(seqChar,"%8d",control[i]);
                    memcpy(sendData,seqChar,8);
                    memcpy(sendData+8,data,sizeof(data));
                    printf("Sending sequence: %d\n",control[i]);
                    n = dropsendto(sockUDP,sendData,sizeof(sendData),(struct sockaddr *)&from,10,9);
                    if (n  < 0) error("sendto");
                }
                else{
                    memcpy(data,&map[control[i]*BLOCKSIZE],finfo.st_size-((numPackets-1)*BLOCKSIZE));
                    sprintf(seqChar,"%8d",control[i]);
                    memcpy(sendData,seqChar,8);
                    memcpy(sendData+8,data,sizeof(data));
                    printf("Sending sequence: %d\n",control[i]);
                    n = dropsendto(sockUDP,sendData,sizeof(sendData),(struct sockaddr *)&from,10,9);
                    if (n  < 0) error("sendto");              
                    }
            }
            return false;
        }
    }
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

    validateCommandLineArguments(argc, argv);
    char * secretKeyGiven = argv[2];
    int sockfd = create_socket_to_listen_on(argv[1]);

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

    printf("The RequestBuf is %s \n", requestBuf);
    if(strcmp(secretKey, secretKeyGiven ) == 0)
    {
        if(validateFile(filename) == true)
        {

            printf("Made it here \n");
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
            printf("Sending File Size: %s\n",buffer);
            client_to_transact_with = (struct sockaddr *)&from;

            unsigned long long int numbytes;
            // loop for initial SYN/ACK 
            while(1) {
                if ((numbytes = dropsendto(sockfd,buffer,sizeof buffer,client_to_transact_with, 10, 9)) == -1) {
                perror("sender: sendto");
                return;
                }
                printf("sender: waiting for ACK from receiver\n");

                char ack_message[4];
                int response_len = recv(sockfd, &ack_message, sizeof(ack_message), 0);
                if (response_len >= 0) {
                    ack_message[3] = '\0';
                    printf("sender: got %s\n", ack_message);
                    break;
                }
                else if (response_len == -1) {
                    perror("sender: receive ACK");
                }
                printf("sender: timeout, retrying.\n");
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

            printf("Starting send\n");

            map = (char*)mmap(0, finfo.st_size, PROT_READ, MAP_SHARED, fd, 0);
            if (map == MAP_FAILED) {
              close(fd);
              error("map");
          }    
          numPackets=((finfo.st_size + BLOCKSIZE -1)/BLOCKSIZE);

          printf("Total numPackets %d \n", numPackets);

          while (1) 
          {

            long int size=finfo.st_size;
            //long int totPkts=(size/1024)+1;
            long int i;
            for(i=0;i<size;i++) {
                char data[BLOCKSIZE];
                char sendData[BLOCKSIZE];
                char seqChar[9];
                if(currSeq == numPackets+1)
                {
                    isFileSent=true;
                    break;
                }

                if (currSeq < numPackets){ //all packets except the last packet
                    sprintf(seqChar,"%8d",currSeq);
                    memcpy(sendData,seqChar,8);
                    memcpy(sendData+8,&map[currSeq*BLOCKSIZE],BLOCKSIZE);
                    printf("Sending sequence: %d\n",currSeq);
                    n = dropsendto(sockUDP,sendData,sizeof(sendData),client_to_transact_with,10,9);
                    if (n  < 0) error("sendto");
                    currSeq++;
                }
                else{ 
                    printf("Sending the last packet\n");
                    sprintf(seqChar,"%8d",currSeq);
                    memcpy(sendData,seqChar,8);
                    memcpy(sendData+8,&map[currSeq*BLOCKSIZE],finfo.st_size-(currSeq*BLOCKSIZE));
                    printf("Sending last sequence: %d\n",currSeq);
                    n = dropsendto(sockUDP,sendData,sizeof(sendData),client_to_transact_with,10,9);
                    if (n  < 0) error("sendto last seq");
                    currSeq++;
                }
            }
            if(isFileSent)
            {
                if(nackhandle(sockUDP, server, fromlen))
                    break;
            }
            }//end while
          // Send the packets one by one of size BLOCKSIZE
          // Compute the checksum of the file and add it to the end of the file later

        //   int num_read = -1;
        //   while((num_read = read(fd,buf,BLOCKSIZE)) > 0){

        //     int x;
        //     for(x =0 ; x < num_read; x++)
        //     {
        //         check_sum+= buf[x];
        //     }
        //     if(num_read==-1){
        //         perror("failed to read file \n ");
        //         return EXIT_FAILURE;
        //     }
        //          // Write out the characters
        //     int num_written = write(fullAssocSockfd, buf, num_read);
        //     if(num_written==-1){
        //         perror("failed to write file to fullAssocSockfd \n");
        //         return EXIT_FAILURE;
        //     }
        //     memset(&buf[0], 0, BLOCKSIZE);
        // }
        //      // convert checksum to big endian as per lab 1
        // check_sum = htobe64(check_sum);
        //      // write out the check sum 
        // write(sockfd, (void *)&check_sum, 8);
             // write out the 0 byte so as to signify end of transmission.
            //write(sockfd, buf, 0);
            close(sockfd);
            exit(EXIT_SUCCESS);
        }
    }
    else
    {
        printf("Secret Key Mismatch Given Key : %s | Expected Key : %s \n", secretKeyGiven, secretKey);
    }
    close(sockfd);
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
        printf("Came here \n");
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
    numSent = sendto(in_fd, message, strlen(message) , 0, who_to_send_addr, addr_len);
    return numSent;
}