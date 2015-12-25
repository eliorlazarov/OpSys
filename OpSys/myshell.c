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
#include "input.c"
int contains(int count, char** arglist, char let) {
	int i;
	for (i = 0; i < count; i++) {
		if (arglist[i][0] == let)
			return 1;
	}
	return 0;
}

int process_arglist(int count, char** arglist) {
	int conAmp = contains(count, arglist, '&');
	int conPipe = contains(count, arglist, '|');
	if(conPipe+conAmp==0)
		execute(arglist, conAmp, conPipe);
	else if (conPipe) {
		np_exec(*arglist)
	}
}

void  execute(char **argv, int conAmp,int conPipe)
{
	pid_t  pid;
	int    status;
	if (conPipe + conAmp == 0) {
		if ((pid = fork()) < 0) {     /* fork a child process           */
			printf("*** ERROR: forking child process failed\n");
			exit(1);
		}
		else if (pid == 0) {          /* for the child process:         */
			if (execvp(*argv, argv) < 0) {     /* execute the command  */
				printf("*** ERROR: exec failed\n");
				exit(1);
			}
		}
		else {                                  /* for the parent:      */
			while (wait(&status) != pid)       /* wait for completion  */
				;
		}
	}
	
}
void np_exec(char* cmd, char** argv)
{
	int pipefd[2];
	assert(pipe(pipefd) != -1);
	int procID = fork();
	if (procID < 0) { // fork failed
		perror("fork failed");
	}
	if (procID == 0) { // child process
		dup2(pipefd[0], fileno(stdin));
		close(pipefd[1]);
	}
	else { // parent process
		dup2(pipefd[1], fileno(stdout));
		close(pipefd[0]);
		if (execvp(cmd, argv) == -1) {
			perror("execvp failed");
		}
	}
}

int execPipe(int argc, char** argv)
{
	assert(strcmp(argv[argc - 1], "-")); // check that the last argument is not "-"

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
	args[argc - 1] = NULL;

	for (i = 1; i < argc; ++i) {
		args[i - 1] = argv[i];
	}

	if (execvp(args[0], args) == -1)
		perror("execvp failed");
	return;
}
