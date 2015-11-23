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

void handler1(int signum){
    unlink(path);
    close(fd);
    exit(1);
}

void handler2(int signum){
    printf("Tried to write to a closed pipe (FIFO file has no reader)\n");
}



void printError(char* c){
 printf("Error in %s:%s\n",c,strerror(errno));
}


int main(int argc, char** argv){
assert ( argc == 2 );
int n;
struct stat statbuf ;
char line[1024];

struct sigaction sig;


sig.sa_handler = handler1;
sigaction(SIGINT, &sig, NULL);
sigaction(SIGTERM, &sig,NULL);


signal(SIGPIPE,handler2);




path=argv[1];

fd=open(path, O_WRONLY);


if(fd<0){
    if(errno==ENOENT){
        n=mkfifo(path,S_IWUSR);
        if(n<0){
            printError("mkfifo");
            return -1;
        }
        fd=open(path,O_WRONLY);
        if(fd==-1){
            printError("open");
            return -1;
        }
    }
    else{
        printError("open");
        return -1;
    }
}

else{
n=stat(path,&statbuf);
if(n<0){
    printError("stat");
    return -1;
}
if(!(S_ISFIFO(statbuf.st_mode))){
    n=unlink(path);
    assert(n>=0);
    n=mkfifo(path,S_IWUSR);
        if(n<0){
            printError("mkfifo");
            return -1;
        }
        fd=open(path,O_WRONLY);
        if(fd<0){
            printError("open");
            return -1;
        }
}
}


while (fgets(line, 1024, stdin) != NULL) {
n = write(fd,line,1024);
if(n<0 && errno!=EPIPE){
    printError("write");
    return -1;
}

}


n = unlink(path);
if(n<0){
    printError("unlink");
    return -1;
}
n = close(fd);
if(n<0){
    printError("close");
    return -1;
}

return 0;

}














