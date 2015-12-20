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
	else if(conAmp)
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
	pthread_t thready;
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
		if(pt=pthread_create(&thready,NULL,waiter,(void *)pid))
		{
			perror("ptread failed");
			exit(0);
		}
	}
	return 1;
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
			perror("execvp failed\n");
			return 0;
		}
	}
	else {
		while (wait(&status) != pid)
			;
	}
	
	return 1;
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
	if((pid1=fork()) < 0){ //Handle the first cmd
		perror("Fork failed");
		exit(1);
	}
	else if (pid1 == 0){
		if(dup2(pipeArr[1],fileno(stdout))<0){
			perror("dup");
			return 0;
		}
		close(pipeArr[0]); 
		close(pipeArr[1]);
		execvp(argv[0], argv);
		perror("execvp failed");//Should only get here in error
		exit(1);
		
	}
	pthread_t t1;
	int returnC=pthread_create(&t1,NULL,waiter,(void*)pid1);
	if(returnC){
		perror("pthread create");
	}
	if((pid2=fork()) < 0){ //Handle the second cmd
		perror("Fork failed");
		exit(1);
	}
	else if (pid2 == 0){
		if(dup2(pipeArr[0],fileno(stdin))<0){
			perror("dup");
			return 0;
		}
		close(pipeArr[1]);
		close(pipeArr[0]);
		execvp(argv2[0], argv2);
		perror("execvp failed");//Should only get here in error
		exit(1);
		
	}
	close(pipeArr[0]);
	close(pipeArr[1]);
	pthread_t t2;
	returnC=pthread_create(&t2,NULL,waiter,(void*)pid2);
	if(returnC){
		perror("pthread create");
	}
	returnC=pthread_join(t1,NULL);
	if(returnC){
		perror("join1");
		exit(1);
	}
	returnC=pthread_join(t2,NULL);
	if(returnC){
		perror("join2");
		exit(1);
	}
	
	return 1;
}
