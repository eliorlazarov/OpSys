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

#define DIRECT 1 //equals 1 if using O_DIRECT flag and 0 otherwise
#define ALIGNED 1 //equals 1 if writing in aligned offset and 0 if unaligned
#define WRITE_SIZE 1024 //the write size in KB (can be changed to 1024/256/64/16/4 KB)

static char buf[1024*1024] __attribute__ ((__aligned__ (4096)));

void printError(char* c) {
	printf("Error in %s:%s\n", c, strerror(errno));
}

int writeToFile(int fd){ //Writes 128MB to the file
	char s[1024 * 1024];
	int i, n;
	for (i = 0; i < 1024 * 1024; i++) {
		s[i] = random() % 256;
	}
	for (i = 0; i < 128; i++) {
		n = write(fd, s, 1024 * 1024);
		if (n == -1) {
			return errno;
		}
		else if (n < 1024 * 1024) {
			printf("PROBLEM: WROTE LESS THAN 1MB");
			exit(0);
		}
	}
	return 0;
}


void buildBuf(){ //puts random chars in buf
	int i;
	for (i = 0; i < 1024 * 1024; i++) {
		buf[i] = random() % 256;
	}
}


int writeInRandomOffsets(int fd, int kb, int aligned){
	int i, offset, n;
	for (i = 0; i < (1024 * 128) / kb; i++) {
		if (aligned) {
			offset = kb * 1024 * (random() % (128 * 1024 / kb)); //random offset depends on kb (if aligned)
		}
		else {
			offset = random() % (128 * 1024 * 1024 - kb * 1024); //random unaligned offset
		}
		n = lseek(fd, offset, SEEK_SET);
		if (n == -1) {
			printError("lseek");
			return errno;
		}

		n = write(fd, buf, kb * 1024);
		if (n == -1) {
			return errno;
		}
		else if (n < kb * 1024) {
			printf("PROBLEM: WROTE LESS THAN %dKB", kb);
			exit(0);
		}

	}
	return 0;
}






int main(int argc, char** argv){
	assert(argc == 2);
	struct timeval startTime, endTime;
	struct stat statbuf;
	long seconds, useconds;
	double mbPerSec, time;
	int errCheck = stat(argv[1], &statbuf), flag = 0, fd;
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

	if (flag == 1) {
		if (S_ISREG(statbuf.st_mode)) {
			printf("It is a regular file\n");
		}
		else if (S_ISBLK(statbuf.st_mode)) {
			printf("It is a block device\n");
			return 0;
		}
		else if (S_ISDIR(statbuf.st_mode)) {
			printf("It is a directory\n");
			return 0;
		}

		if (statbuf.st_size != 1024 * 1024 * 128) {
			fd = open(argv[1], O_TRUNC | O_RDWR);
			errCheck = writeToFile(fd);
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
	

	buildBuf();

	gettimeofday(&startTime, NULL);

	if (DIRECT) 
		fd = open(argv[1], O_DIRECT | O_RDWR);
	else 
		fd = open(argv[1], O_RDWR);

	if (fd == -1) {
		printError("open");
		return 0;
	}
	if (writeInRandomOffsets(fd, WRITE_SIZE, ALIGNED) == -1) 
		return 0;
	close(fd);
	gettimeofday(&endTime, NULL);
	seconds = endTime.tv_sec - startTime.tv_sec;
	useconds = endTime.tv_usec - startTime.tv_usec;
	time = seconds + useconds / 1000000.0;
	mbPerSec = ((WRITE_SIZE * 1024)*((1024 * 128) / WRITE_SIZE)) / (1024 * 1024 * time);


	if (WRITE_SIZE != 1024) {

		if (ALIGNED && DIRECT) {
			printf("Aligned with O_DIRECT: WRITE_SIZE=%dKB , total time=%f sec , throughput=%f MB/sec ", WRITE_SIZE, time, mbPerSec);
		}
		else if (ALIGNED && DIRECT == 0) {
			printf("Aligned non-direct : WRITE_SIZE=%dKB , total time=%f sec , throughput=%f MB/sec ", WRITE_SIZE, time, mbPerSec);
		}
		else if (ALIGNED == 0 && DIRECT) {
			printf("Unaligned with O_DIRECT : WRITE_SIZE=%dKB , total time=%f sec , throughput=%f MB/sec ", WRITE_SIZE, time, mbPerSec);
		}
		else {
			printf("Unaligned non-direct : WRITE_SIZE=%dKB , total time=%f sec , throughput=%f MB/sec ", WRITE_SIZE, time, mbPerSec);
		}
	}
	else {
		if (ALIGNED && DIRECT) {
			printf("Aligned with O_DIRECT: WRITE_SIZE=1MB , total time=%f sec , throughput=%f MB/sec ", time, mbPerSec);
		}
		else if (ALIGNED && DIRECT == 0) {
			printf("Aligned non-direct : WRITE_SIZE=1MB , total time=%f sec , throughput=%f MB/sec ", time, mbPerSec);
		}
		else if (ALIGNED == 0 && DIRECT) {
			printf("Unaligned with O_DIRECT : WRITE_SIZE=1MB , total time=%f sec , throughput=%f MB/sec ", time, mbPerSec);
		}
		else {
			printf("Unaligned non-direct : WRITE_SIZE=1MB , total time=%f sec , throughput=%f MB/sec ", time, mbPerSec);
		}
	}


	return 0;



}
