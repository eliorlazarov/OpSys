#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h> // for open flags
#include <time.h> // for time measurement
#include <assert.h>
#include <errno.h> 
#include <string.h>


int main(int argc, char* argv[])
{

	char temp;
	char currKey;
	char curr;
	char* buf[1];
	if (argc != 4){
		printf("Not enough arguments/n");
		return 1;
	}
	char buf[1];
	int fd = open(argv[1]);
	int fdWrite = open(argv[3]);

	if (fd < 0){
		printf("Error opening file: %s\n", strerror(errno));
		return errno;
	}
	if (fdWrite < 0){
		printf("Error opening file: %s\n", strerror(errno));
		return errno;
	}
	int fdKey = open(argv[2]);
	if (fdKey < 0){
		printf("Error opening file: %s\n", strerror(errno));
		return errno;
	}
	while (curr = read(fd, buf, 1) == 1 && curr != EOF){
		currKey = lseek(fdKey, 1, SEEK_CUR);
		if (currKey == EOF)
			currKey = lseek(fdKey, 0, SEEK_SET);
		buf[0] = (currKey^curr);
		write(fdWrite, buf, 1);
	}
}