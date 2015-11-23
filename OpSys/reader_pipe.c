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

void printError(char* c){
 printf("Error in %s:%s\n",c,strerror(errno));
}



int main(int argc, char** argv){
assert ( argc == 2 );
int n;
struct stat statbuf ;
int fd;
char line[1024];
struct sigaction prev;


while(1){

fd=open(argv[1],O_RDONLY);


while(fd<0){
        if(errno=ENOENT){
            sleep(1);
            fd=open(argv[1],O_RDONLY);
        }
        else{
            printError("open");
            return -1;
        }
}

n = stat(argv[1],&statbuf);
if(n<0){
    printError("stat");
    return -1;
}

if(!(S_ISFIFO(statbuf.st_mode))){
    printf("It is not a FIFO file!\n");
    return -1;
}
sigaction(SIGINT,NULL, &prev);
sigaction(SIGTERM,NULL, &prev);



while (read(fd,line,1024) > 0) {
printf("%s",line);
}

n = close(fd);



sigaction(SIGINT,&prev,NULL);
sigaction(SIGTERM,&prev,NULL);

if(n<0){
    printError("close");
    return -1;
}

}
return 0;

}






