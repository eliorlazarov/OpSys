#include <signal.h>
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

int main(int argc, char** argv) {
	assert(argc == 2);
	struct stat statbuf;
	int fd;
	char line[1024];
	struct sigaction prev;


	while (1) {
		while (fd = open(argv[1], O_RDONLY) < 0) {
			if (errno == ENOENT) {
				sleep(1);
				fd = open(argv[1], O_RDONLY);
			}
			else {
				printf("Error opening file, quiting.\n");
				return -1;
			}
		}
		if (stat(argv[1], &statbuf) < 0) {
			printf("Error- using stat failed. quiting.\n");
			return -1;
		}
		if (!(S_ISFIFO(statbuf.st_mode))) {
			printf("It is not a FIFO file!\n");
			return -1;
		}
		sigaction(SIGINT, NULL, &prev);
		sigaction(SIGTERM, NULL, &prev);
		while (read(fd, line, 1024) > 0) {
			printf("%s", line);
		}
		if (close(fd) < 0) {
			printf("Error, close file failed. quiting\n");
			sigaction(SIGINT, &prev, NULL);
			sigaction(SIGTERM, &prev, NULL);
			return -1;
		} 
		sigaction(SIGINT, &prev, NULL);
		sigaction(SIGTERM, &prev, NULL);
	}
	return 0;

}