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
int execute(char **argv);
int pipeExec(char** argv,int i);

void* waiter(void* arg){
	int status;
	int arg1=(int) arg;
	waitpid((int)arg1,&status,1);
}


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
	if(conPipe)
		pipeExec(arglist,conPipe);
	if(conAmp)
		ampExec(arglist,count);
	else
		execute(arglist);
	
	return 1;
}

int ampExec(char **argv,int count){
	int pt;
	pid_t pid;
	int status;
	argv[count-1]=NULL;
	
	if ((pid = fork()) < 0) {
		perror("Fork failed");
		exit(1);
	}
	else if (pid == 0) {//If child
		if (execvp(*argv, argv) < 0) {
			perror("exec failed\n");
			return 0;
		}
	}
	else{
		pt=pthread_create(pt,NULL,waiter,(void *)pid);
	}
	return 0;
}

int execute(char **argv){
	
	pid_t pid;
	int status;
	
	
	if ((pid = fork()) < 0) {
		perror("Fork failed");
		exit(1);
	}
	else if (pid == 0) {//If child
		if (execvp(*argv, argv) < 0) {
			perror("exec failed\n");
			return 0;
		}
	}
	else {
		while (wait(&status) != pid)
			;
	}
	
	
}
//Based on stackoverflow code
int pipeExec(char** argv,int i){
	
	pid_t pid1,pid2;
	int pipeArr[2];
	argv[i]=NULL;
	char** argv2=argv+i+1;//Setting the second command
	if(pipe(pipeArr) == -1) {
		perror("Pipe failed");
		exit(1);
	}
	
	if(pid1=fork() == 0) //Handle the first cmd
	{//Closes stdout first in order to write to the pipe instead of stdout
		close(1);
		dup2(pipeArr[1],fileno(stdout));
		
		close(pipeArr[0]); 
		close(pipeArr[1]);
		execvp(argv[0], argv);
		perror("execvp failed");//Should only get here in error
		exit(1);
	}
	else if (pid1<0){
		perror("Fork failed");
		exit(1);
	}
	if(pid2=fork() == 0) //Handle the second cmd
	{//Closes stdin first in order to write to the pipe instead of stdin
		close(0);
		dup2(pipeArr[0],fileno(stdin));
		close(pipeArr[1]);
		close(pipeArr[0]);
		execvp(argv2[0], argv2);
		perror("execvp failed");//Should only get here in error
		exit(1);
	}
	else if (pid2<0){
		perror("Fork failed");
		exit(1);
	}
	close(pipeArr[0]);
	close(pipeArr[1]);
	wait(0);
	wait(0);
	return 0;
}
