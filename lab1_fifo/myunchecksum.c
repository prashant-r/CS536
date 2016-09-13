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
	last8[read_byte] = '\0';
	printf("Last 8 %s\n", last8);

	// remove from check_sum
	for(int a = 0 ; a < 8 ; a++)
		check_sum -= last8[a];


	unsigned long long last8converted = be64_to_cpup(&last8);

	printf("\nGiven Checksum %llu \n", last8converted);

	printf("\nObtained Checksum %llu\n", check_sum);

	close(fd);
	_exit(fd);


	return 0;
}


unsigned long long swap_ull( unsigned long long val)
{
    val = ((val << 8) & 0xFF00FF00FF00FF00ULL ) | ((val >> 8) & 0x00FF00FF00FF00FFULL );
    val = ((val << 16) & 0xFFFF0000FFFF0000ULL ) | ((val >> 16) & 0x0000FFFF0000FFFFULL );
    return (val << 32) | (val >> 32);
}
