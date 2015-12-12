#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h> // for open flags
#include <time.h> // for time measurement
#include <assert.h>
#include <errno.h>
#include <string.h>

void np_exec(char* cmd, char** argv)
{
	int pipefd[2];
	assert(pipe(pipefd) != -1);
	int procID = fork();
	if(procID < 0){ // fork failed
		perror("fork failed");
	}
	if(procID == 0){ // child process
		dup2(pipefd[0], fileno(stdin));
		close(pipefd[1]);
	}
	else{ // parent process
		dup2(pipefd[1], fileno(stdout));
		close(pipefd[0]);
		if(execvp(cmd, argv) == -1){
			perror("execvp failed");
		}
	}
}

int main(int argc, char** argv)
{
	assert(strcmp(argv[argc-1], "-")); // check that the last argument is not "-"

	int i;
	for (i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "-")) { // if argv[i] is "-"
			argv[i] = NULL;
			np_exec(argv[1], &argv[1]);
			argv = &argv[i];
			argc -= i;
			i = 0;
		}
	}

	char* args[argc];
	args[argc-1] = NULL;
	
	for (i = 1; i < argc; ++i) {
		args[i-1] = argv[i];
	}
	
	if (execvp(args[0], args) == -1)
		perror("execvp failed");
	return;
}