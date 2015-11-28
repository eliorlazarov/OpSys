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

void handler1(int signum) {
	unlink(path);
	close(fd);
	exit(1);
}

void handler2(int signum) {
	printf("Tried to write to a closed pipe\n");
}

void printErr(char* c) {
	printf("Error in %s:%s\n", c, strerror(errno));
}

int main(int argc, char** argv) {
	assert(argc == 2);
	struct stat statbuf;
	char line[1024];
	
	struct sigaction sig;
	sig.sa_handler = handler1;
	sigaction(SIGINT, &sig, NULL);
	sigaction(SIGTERM, &sig, NULL);

	struct sigaction action_Pipe;
	action_Pipe.sa_handler = handler2;
	sigaction(SIGPIPE, &action_Pipe, NULL);
	path = argv[1];
	
	fd = open(path, O_WRONLY);
	if (fd < 0) {
		if (errno == ENOENT) {
			
			if (mkfifo(path, S_IWUSR | S_IRUSR |S_IRGRP | S_IROTH) < 0) {
				printErr("mkfifo");
				return -1;
			}
			if ((fd = open(path, O_WRONLY)) == -1) {
				printErr("open");
				return -1;
			}
		}
		else {
			printErr("open");
			return -1;
		}
	}
	
	
	if (stat(path, &statbuf) < 0) {
		printErr("stat");
		return -1;
	}
	if (!(S_ISFIFO(statbuf.st_mode))) {
		assert(unlink(path) >= 0);
		if (mkfifo(path, S_IWUSR | S_IRUSR |S_IRGRP | S_IROTH) < 0) {
			printErr("mkfifo");
			return -1;
		}
		if (fd = open(path, O_WRONLY) < 0) {
			printErr("open");
			return -1;
		}
		
	}
	
	while (fgets(line, 1024, stdin) != NULL) {
		if (write(fd, line, 1024) < 0 && errno != EPIPE) {
			printErr("write");
			return -1;
		}
	}
	if (unlink(path) < 0) {
		printErr("unlink");
		return -1;
	}
	if (close(fd) < 0) {
		printErr("close");
		return -1;
	}
	return 0;
	
}
