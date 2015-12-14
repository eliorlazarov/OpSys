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
			return i;
	}
	return 0;
}

int process_arglist(int count, char** arglist) {
	int i;
	int conAmp = contains(count, arglist, '&');
	int conPipe = contains(count, arglist, '|');
	if(conPipe){
		np_exec(arglist,conPipe);
	}
	else
		execute(arglist, conAmp, conPipe);
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
void np_exec(char** argv,int i)
{
	int des_p[2];
	argv[i]=NULL;
	char** argv2=argv+i+1;
	if(pipe(des_p) == -1) {
		perror("Pipe failed");
		exit(1);
	}
	
	if(fork() == 0)        //first fork
	{
		close(1);          //closing stdout
		dup(des_p[1]);     //replacing stdout with pipe write 
		close(des_p[0]);   //closing pipe read
		close(des_p[1]);
		
		
		execvp(argv[0], argv);
		perror("execvp of ls failed");
		exit(1);
	}
	
	if(fork() == 0)        //creating 2nd child
	{
		close(0);          //closing stdin
		dup(des_p[0]);     //replacing stdin with pipe read
		close(des_p[1]);   //closing pipe write
		close(des_p[0]);
		
		
		execvp(argv2[0], argv2);
		perror("execvp of wc failed");
		exit(1);
	}
	
	close(des_p[0]);
	close(des_p[1]);
	wait(0);
	wait(0);
	return 0;
}
