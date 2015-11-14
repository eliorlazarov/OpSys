#define _GNU_SOURCE
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



static char buf[1024*1024] __attribute__ ((__aligned__ (4096)));

void printError(char* c) {
	printf("Error in %s:%s\n", c, strerror(errno));
}

int writeToFile(int fd){
	char buf[1024 * 1024];
	int readSize;
	int i, j;
	for (i = 0; i < 1024 * 1024; i++) {
		buf[i] = random() % 256;
	}
	for ( j = 0; j < 128; j++) {
		readSize = write(fd, buf, 1024 * 1024);
		if (readSize == -1) {
			return errno;
		}
		else if (readSize < 1024 * 1024) {
			printf("PROBLEM: WROTE LESS THAN 1MB");
			exit(0);
		}
	}
	return 0;
}


int writeInRandomOffsets(int fd, int writeSize){
	int offset, size;
	int i;
	for ( i = 0; i < (1024 * 128) / writeSize; i++) {
		offset = writeSize * 1024 * (random() % (128 * 1024 / writeSize)); //random offset depends on kb (if aligned)
		
		size = lseek(fd, offset, SEEK_SET);
		if (size == -1) {
			
			return errno;
		}
		
		size = write(fd, buf, writeSize * 1024);
		if (size == -1) {
			return errno;
		}
		else if (size < writeSize * 1024) {
			printf("not enough was written, quitting.\n");
			exit(0);
		}
		
	}
	return 0;
}

int main(int argc, char** argv) {
	assert(argc == 4);
	struct timeval startTime, endTime;
	struct stat statbuf;
	long seconds, useconds;
	double mbPerSec, time;
	int errCheck = stat(argv[1], &statbuf), flag = 0, fd;
	int direct, writeSize;
	int j;
	if (errCheck == -1)
	{
		if (errno == ENOENT) {
			printf("Input file does not exist\n");
		}
		else {
			printError("stat");
			return errno;
		}
	}
	else {
		printf("Input file exists\n");
		flag = 1;
	}
	int isBlock=0;
	if (flag == 1) {
		if (S_ISBLK(statbuf.st_mode)) {
			printf("It is a block device\n");
			isBlock=1;
		}
		else{
			if (S_ISREG(statbuf.st_mode)) {
				printf("It is a regular file\n");
			}
			
			else if (S_ISLNK(statbuf.st_mode)) {
				printf("It is a link\n");
				return 0;
			}
			else if (statbuf.st_nlink > 2) {
				printf("It has links\n");
				return 0;
			}
			
			if (statbuf.st_size != 1024 * 1024 * 128 ) {
				fd = open(argv[1], O_TRUNC | O_RDWR);
				errCheck = writeToFile(fd);
			}
		}
	}
	else {
		fd = creat(argv[1], S_IRWXU);
		errCheck = writeToFile(fd);
	}
	if (errCheck == -1) {
		printError("write");
		return 0;
	}
	else
		close(fd);
	
	
	for ( j = 0; j < 1024 * 1024; j++) {
		buf[j] = random() % 256;
	}
	double avgTime=0, avgT=0;
	int h;
	direct = atoi(argv[2]);
	writeSize = atoi(argv[3]);
	for(h=0;h<5;h++){
		gettimeofday(&startTime, NULL);
		
		if (direct)
			fd = open(argv[1], O_DIRECT | O_RDWR);
		else
			fd = open(argv[1], O_RDWR);
		
		if (fd == -1) {
			printError("open");
			return 0;
		}
		if (writeInRandomOffsets(fd, writeSize) == -1)
			return 0;
		close(fd);
		gettimeofday(&endTime, NULL);
		seconds = endTime.tv_sec - startTime.tv_sec;
		useconds = endTime.tv_usec - startTime.tv_usec;
		time = seconds + useconds / 1000000.0;
		time*=1000;
		mbPerSec = 128*1000/time;
		avgTime+=time;
		avgT+=mbPerSec;
	}
	avgT/=5;
	avgTime/=5;
	
	
	if (direct) {
		printf("with O_direct: writeSize=%dKB , total time=%f MS , throughput=%f MB/sec\n", writeSize, avgTime, avgT);
	}
	
	else {
		printf("non-direct : writeSize=%dKB , total time=%f MS , throughput=%f MB/sec\n", writeSize, avgTime, avgT);
	}
	
	
	return 0;
}