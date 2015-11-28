#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

int fd;
char *path;

void IntTermHandler(int signum){

	assert(unlink(path) >= 0); // removes the FIFO file
	assert(close(fd) >= 0); // closes the FIFO file
	exit(-1);
}

void PipeHandler(int signum){
	printf("Writing to a closed pipe.\n");
}

int main(int argc, char** argv){
	
	assert(argc == 2);
	struct stat stat_buf;
	path = argv[1];

	struct sigaction action_IntTerm;
	action_IntTerm.sa_handler = IntTermHandler;
	sigaction(SIGINT, &action_IntTerm, NULL);
	sigaction(SIGTERM, &action_IntTerm, NULL);

	struct sigaction action_Pipe;
	action_Pipe.sa_handler = PipeHandler;
	sigaction(SIGPIPE, &action_Pipe, NULL);

	fd = open(path, O_WRONLY, S_IRWXO|S_IRWXG|S_IRWXU);
	if(fd < 0){ 
		if(errno == ENOENT){ // file does not exist
			
			if(mkfifo(path, S_IRWXO|S_IRWXG|S_IRWXU) < 0){ // error occurred
				printf("Error occurred.\n");
				return -1;
			}
			fd = open(path, O_WRONLY, S_IRWXO|S_IRWXG|S_IRWXU);
		}
		else{
			printf("Error occurred.\n");
			return -1;
		}
	}
	else{ // file exists
		if(fstat(fd, &stat_buf) < 0){
			printf("Error occurred.\n");
			return -1;
		}
		if ((stat_buf.st_mode & S_IFMT) != S_IFIFO) { // not a FIFO file
			
			assert(unlink(path) >= 0);

			if(mkfifo(path, S_IRWXO|S_IRWXG|S_IRWXU) < 0){ // error occurred
				printf("Error occurred.\n");
				return -1;
			}
			fd = open(path, O_WRONLY, S_IRWXO|S_IRWXG|S_IRWXU);	
        }
	}

	char line[1024];

	while (fgets(line, 1024, stdin) != NULL) {
		write(fd, line, 1024);
	}

	assert(unlink(path) >= 0); // removes the FIFO file
	assert(close(fd) >= 0); // closes the FIFO file

	return 0;
}