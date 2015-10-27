#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h> // for open flags
#include <time.h> // for time measurement
#include <assert.h>
#include <errno.h> 
#include <string.h>


int main(int argc, char* argv[])
{
	struct dirent *dp;
	int fd,fdWrite;
	int place;
	char temp;
	char currKey;
	char curr='a';
	char buf[1];
	char bufKey[1];
	DIR* dfd;
	DIR* dirWrite;
	//char* dir,write;
	int k;
	char fileName[100];
	char fileWrite[100];
	if (argc != 4){
		printf("Not enough arguments\n");
		return 1;
	}
	
	if((dfd=opendir(argv[1]))==NULL){
	  
	//int fd = open(argv[1],O_RDONLY);
	printf("Error opening dir: %s\n", strerror(errno));
		return errno;
	}


	if((dirWrite=opendir(argv[3]))==NULL){
	  printf("Error opening dir: %s\n", strerror(errno));
		return errno;
	}

	int fdKey = open(argv[2],O_RDONLY);

	if (fdKey < 0){
		printf("Error opening file: %s\n", strerror(errno));
		close(fd);
		close(fdWrite);
		return errno;
	}
		
	while((dp=readdir(dfd))!=NULL){
	  struct stat statbuf;
	sprintf(fileName,"%s/%s",argv[1],dp->d_name);

	if((statbuf.st_mode & S_IFMT)==S_IFDIR || !(strcmp(dp->d_name,".")) || !(strcmp(dp->d_name,"..")))
	  continue;
	
	  else{
	if(stat(fileName,&statbuf)==-1){
	  printf("Error reading file: %s\n", strerror(errno));
		close(fd);
		close(fdWrite);
		return errno;
	}
	  char str[100];
	  strcpy(str,argv[3]);
	   strcat(str,"/");
	  strcat(str,dp->d_name);
	  	fdWrite = open(str,O_WRONLY | O_CREAT | O_TRUNC);
	if (fdWrite < 0){
		printf("Error opening file: %s\n", strerror(errno));
		close(fd);
		return errno;
	}
	  fd = open(fileName,O_RDONLY);
	  if(fd<0){
		printf("Error opening file: %s\n", strerror(errno));
		close(fd);
		close(fdWrite);
		return errno;
	}
	}
	int i=0;
	while (read(fd, buf, 1) == 1 && curr != EOF){
	  k=read(fdKey,bufKey,1);
		if(k<0){
		  printf("Error reading file: %s\n",strerror(errno));

		  close(fd);
		  close(fdWrite);
		  close(fdKey);
		  return errno;
		}
		else if (k==0){
		  lseek(fdKey, 0, SEEK_SET);
		  k=read(fdKey,bufKey,1);
		  if(k<0){
		  printf("Error reading file: %s\n",strerror(errno));

		  close(fd);
		  close(fdWrite);
		  close(fdKey);
		  return errno;
		}
		}
		
		buf[0] = (bufKey[0] ^ buf[0]);
		
		if(write(fdWrite, buf, 1)!=1){
		  printf("Error writing file: %s\n",strerror(errno));
		  close(fd);
		  close(fdWrite);
		  close(fdKey);
		  return errno;
		}

	}
	}
    close(fd);
    close(fdWrite);
    close(fdKey);
    return 0;
} 
