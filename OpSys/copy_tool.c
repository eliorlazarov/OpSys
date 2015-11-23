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
#include <sys/mman.h>
#include <unistd.h>


int main(int argc, char** argv) {
	assert(argc == 3);
	int fd1 = open(argv[1], O_RDWR), fd2;
	int size, t;
	char *arr1, *arr2;
	
	
	if (fd1 < 0) {
		if (errno == ENOENT) {
			printf("Source file doesn't exist!\n");
		}
		else {
			printf("Error opening file, quiting.\n");
		}
		return -1;
	}
	
	
	fd2 = open(argv[2], O_CREAT | O_EXCL | O_RDWR, S_IRWXO | S_IRWXG | S_IRWXU);
	
	if (fd2 < 0 && errno == EEXIST) {
		printf("The destination file already exists!\n");
		return -1;
	}
	else if (fd2 < 0) {
		printf("Error opening file, quiting.\n");
		return -1;
	}
	;
	
	if ((size = lseek(fd1, 1, SEEK_END)) == -1) {
		printf("Error while using lseek, quiting.\n");
		close(fd1);
		close(fd2);
		unlink(argv[2]);
		return -1;
	}
	
	if ((t=truncate(argv[2], size)) == -1) {
		close(fd1);
		close(fd2);
		unlink(argv[2]);
		printf("Error using truncate, quiting.\n");
	}
	
	arr1 = (char*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd1, 0);
	
	if (arr1 == MAP_FAILED) {
		close(fd1);
		close(fd2);
		unlink(argv[2]);
		printf("Error mmapping the source file, quiting.\n");
		return -1;
	}
	
	arr2 = (char*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd2, 0);
	
	if (arr2 == MAP_FAILED) {
		close(fd1);
		close(fd2);
		unlink(argv[2]);
		printf("Error mmapping the destination file, quiting.\n");
		return -1;
	}
	
	for (t = 0; t < size; t++) {
		arr2[t] = arr1[t];
	}
	
	if (munmap(arr1, size) == -1) {
		printf("Error un-mmapping the source file\n");
		close(fd1);
		close(fd2);
		unlink(argv[2]);
		return -1;
	}
	
	
	if (munmap(arr2, size) == -1) {
		printf("Error un-mmapping the destination file\n");
		close(fd1);
		close(fd2);
		unlink(argv[2]);
		return -1;
	}
	
	if (close(fd1) < 0) {
		printf("Error closing, quiting.\n");
		return -1;
	}
	
	if (close(fd2) < 0) {
		printf("Error closing, quiting.\n");
		return -1;
	}
	return 0;
	
}
