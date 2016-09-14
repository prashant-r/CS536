#define _DEFAULT_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <stdint.h>

int main(int argc, char **argv){

	if( argc == 3 ) {
	   }
	   else if( argc > 3 ) {
	      printf("Too many arguments supplied.\n");
	      return -1;
	   }
	   else {
	      printf("Two arguments expected.\n");
	      return -1;
	   }

	char c;
	unsigned long long check_sum =0;
	int fd, fdw;
	fd = open(argv[1], O_RDONLY);
	if (fd == -1) {
	   printf("Error opening file\n");
	   return -1;
	}
	int counter = 0;
	char buffer[1];
	ssize_t nrd;
	char last8[8];
	while ((nrd = read(fd, buffer, 1)))
	{
		check_sum += buffer[0];
	}

	int read_byte;
	lseek(fd, -8L, SEEK_END);
	read_byte = read(fd, last8, 8);
	printf("Last 8 %s\n", last8);

	// remove from check_sum
	int a;
	for(a = 0 ; a < 8 ; a++)
	{
		check_sum -= last8[a];

	}
	uint64_t small_check_sum = (*(uint64_t *) last8);
	unsigned long long lhs = (unsigned long long ) be64toh(small_check_sum);
	unsigned long long rhs = check_sum;
	if(lhs == rhs)
	{
		printf("Verdict: %s \n", "match");
		printf("Note: File2 will NOT be copied to file1 if match \n");
	}
	else
	{
		printf("Verdict: %s\n","does not match");
		printf("LHS(checksum signature): %llx | RHS(computed checksum): %llx\n", lhs, rhs);
		// strip last 8 bytes
		FILE *fp = fopen (argv[1], "rb");
		fseek (fp , -8 , SEEK_END );
		size_t sizeToRead=ftell (fp);
		rewind (fp);

		// make a buffer to store the entire file
		char * buffer = (char*) malloc (sizeof(char)*sizeToRead);
		if (buffer == NULL) {fputs ("Memory error",stderr); exit (2);}

		int resultant_read_size = fread (buffer,1,sizeToRead,fp);
		if (resultant_read_size != sizeToRead) {fputs ("Reading error",stderr); exit (3);}


		// port the entire file over the file1.
		int fdw = open(argv[2], O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
		write(fdw, buffer, sizeof(char)*sizeToRead);

		printf("File has been copied from file2 to file1!!\n");

		// close the file pointer free the buffer
		fclose (fp);
		free (buffer);

		// exit the program successfully
		close(fdw);
		_exit(fdw);
	}

	close(fd);
	_exit(fd);


	return 0;
}
