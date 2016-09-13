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
	//Read the first file
	uint64_t check_sum =0;
	int fd, fdw;
	fd = open(argv[1], O_RDONLY);
	fdw = open(argv[2], O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);

	char buffer[1];
	ssize_t nrd;
	while ((nrd = read(fd, buffer, 1)))
	{
		check_sum += buffer[0];
		write(fdw, buffer, nrd);
	}

	check_sum = htobe64(check_sum);

	char buf[8];
	int c = snprintf(buf, 8, "%llu", check_sum);
	write(fdw, buf, 8);
	close(fd);
	close(fdw);
	_exit(fd);
	return 0;
}

