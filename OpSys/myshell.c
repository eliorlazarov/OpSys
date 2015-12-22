/*checks if the list had & or |.
for each case, run the right command.
if it has &, run the command on a child process, 
and wait for it in the father proc to prevent zombies.
if it has |, run each command on a different child
connect the stdout and stdin so the first commands prints
to the second command and not to the screen.
wait for both proc to prevent zombies, and join the threads in the end.
if it doesn't have either, just run the command as-is
*/


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

//the waiting function
void* waiter(void* arg){
	int status;
	int arg1=(int) arg;
	waitpid((int)arg1,&status,1);
}

//checks if it has a specific symobl and return it's location
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
//For the & case
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
		if (execvp(argv[0], argv) < 0) {
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
//For the regular case
int execute(char **argv){
	
	pid_t pid;
	int status;
	
	
	if ((pid = fork()) < 0) {
		perror("Fork failed");
		exit(1);
	}
	if (pid == 0) {//If child
		if (execvp(argv[0], argv) < 0) {
			perror("execvp failed\n");
			return 0;
		}
	}
	else {//Parent
		while (1)
			if(wait(&status) != pid);
				break;
	}
	
	return 1;
}
//with some help from stackoverflow
//for the | case
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
		exit(0);
		
	}
	pthread_t t1;
	int r=pthread_create(&t1,NULL,waiter,(void*)pid1);
	if(r){
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
		exit(0);
		
	}
	close(pipeArr[0]);
	close(pipeArr[1]);
	pthread_t t2;
	r=pthread_create(&t2,NULL,waiter,(void*)pid2);
	if(r){
		perror("pthread create");
	}
	r=pthread_join(t1,NULL);
	if(r){
		perror("join1");
		exit(1);
	}
	r=pthread_join(t2,NULL);
	if(r){
		perror("join2");
		exit(1);
	}
	
	return 1;
}
