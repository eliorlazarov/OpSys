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
	int place;
	char temp;
	char currKey;
	char curr;
	char buf[1];
	char bufKey[1];
	int k;
	if (argc != 4){
		printf("Not enough arguments\n");
		return 1;
	}
	int fd = open(argv[1],O_RDONLY);
	

	if (fd < 0){
		printf("Error opening file: %s\n", strerror(errno));
		return errno;
	}
	int fdWrite = open(argv[3],O_WRONLY);
	if (fdWrite < 0){
		printf("Error opening file: %s\n", strerror(errno));
		close(fd);
		return errno;
	}
	int fdKey = open(argv[2],O_RDONLY);
	if (fdKey < 0){
		printf("Error opening file: %s\n", strerror(errno));
		close(fd);
		close(fdWrite);
		return errno;
	}
	while (read(fd, buf, 1) == 1 && curr != EOF){
	  k=read(fdKey,bufKey,1);
		if(k<0){
		  printf("Error reading file: %s\n",strerror(errno));

		  close(fd);
		  close(fdWrite);
		  close(fdKey);
		  return errno;
		}
		else if (k==0){
		  lseek(fdKey, 0, SEEK_SET);
		  k=read(fdKey,bufKey,1);
		}
		
		buf[0] = (bufKey[0] ^ buf[0]);
		if(write(fdWrite, buf, 1)!=1){
		  printf("Error writing file: %s\n",strerror(errno));
		  close(fd);
		  close(fdWrite);
		  close(fdKey);
		  return errno;
		}

	}
    close(fd);
    close(fdWrite);
    close(fdKey);
    return 0;
} 
