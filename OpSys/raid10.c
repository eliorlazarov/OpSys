// usage example:
// sudo ./raid0 /dev/sdb /dev/sdc

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h> // for open flags
#include <assert.h>
#include <errno.h> 
#include <string.h>
#include <unistd.h>
#define DEVICE_SIZE (1024*1024*20) // assume all devices identical in size

#define SECTOR_SIZE 512
#define SECTORS_PER_BLOCK 4
#define BLOCK_SIZE (SECTOR_SIZE * SECTORS_PER_BLOCK)

#define BUFFER_SIZE (BLOCK_SIZE * 2)
#define DEBUG 1
char	buf[BUFFER_SIZE];
int		num_dev0;
int		num_dev1;
int		**dev_fd;


int findWorkingDevice(int dev0) {
	int i;
	for ( i = 0; i < num_dev1; i++) {
		if (dev_fd[dev0][i] >= 0)
			return i;
	}
	
	return -1;
}

void closeAll() {
	int i, j;
	for ( i = 0; i < num_dev0; ++i) {
		for ( j = 0; j < num_dev1; j++) {
			if (dev_fd[i][j] > 0)
				close(dev_fd[i][j]);
		}
	}
	for ( i = 0; i < num_dev0; i++)
		free(dev_fd[i]);
	free(dev_fd);
}

void do_raid0_rw(char* operation, int sector, int count)
{
	int i = sector;
	int j;
	while (i < sector + count)
	{
		// find the relevant device for current sector
		int block_num = i / SECTORS_PER_BLOCK;
		int dev_num = block_num % num_dev0;
		
		// make sure device didn't fail
		int defaultDev = findWorkingDevice(dev_num);
		if (defaultDev == -1)
		{
			printf("No working drives\n");
			break;
		}
		// find offset of sector inside device
		int block_start = i / (1 * SECTORS_PER_BLOCK);
		
		int block_off = i % SECTORS_PER_BLOCK;
		int sector_start = block_start * SECTORS_PER_BLOCK + block_off;
		int offset = sector_start * SECTOR_SIZE;
		
		// try to write few sectors at once
		int num_sectors = SECTORS_PER_BLOCK - block_off;
		while (i + num_sectors > sector + count)
			--num_sectors;
		int sector_end = sector_start + num_sectors - 1;
		int size = num_sectors * SECTOR_SIZE;
		
		// validate calculations
		assert(num_sectors > 0);
		assert(size <= BUFFER_SIZE);
		assert(offset + size <= DEVICE_SIZE);
		// seek in relevant device
		if (offset != lseek(dev_fd[dev_num][defaultDev], offset, SEEK_SET)) {
			printf("There's been an error while using lseek\n");
			close(dev_fd[dev_num][defaultDev]);
			dev_fd[dev_num][defaultDev] = -1;
		}
		
		if (!strcmp(operation, "READ")) {
			while (1) {
				if (size != read(dev_fd[dev_num][defaultDev], buf, size)) {
					printf("Something went wrong while reading\n");
					close(dev_fd[dev_num][defaultDev]);
					dev_fd[dev_num][defaultDev] = -1;
					defaultDev = findWorkingDevice(dev_num);
					if (defaultDev == -1)
					{
						printf("No working drives, can't continue\n");
						return;
					}
				}
				else {
					printf("Operation on device %d, sector %d-%d\n",
					       dev_num*num_dev1 + defaultDev, sector_start, sector_end);
					
					break;
				}
			}
			
			
		}
		
		else if (!strcmp(operation, "WRITE"))
			for ( j = 0; j < num_dev1; j++) {
				if (dev_fd[dev_num][j] >= 0)
					if (size != write(dev_fd[dev_num][j], buf, size)) {
						printf("Something went wrong while writing\n");
						close(dev_fd[dev_num][j]);
						dev_fd[dev_num][j] = -1;
					}
					else
						printf("Operation on device %d, sector %d-%d\n",
						       dev_num*num_dev1 + defaultDev, sector_start, sector_end);
			}
			i += num_sectors;
	}
}


int main(int argc, char** argv)
{
	assert(argc > 2);
	
	int i, j;
	char line[1024];
	char rep[1024];
	// number of devices == number of arguments (ignore 1st)
	
	num_dev0 = atoi(argv[1]);
	
	num_dev1 = (argc - 2) / num_dev0;
	
	int _dev_fd[num_dev0][num_dev1];
	
	
	
	dev_fd = (int**)malloc(num_dev0*sizeof(int*));
	for ( i = 0; i < num_dev0; i++)
		dev_fd[i] = (int*)malloc(num_dev1*sizeof(int));
	// open all devices
	for (i = 0; i < num_dev0; i++) {
		for ( j = 0; j < num_dev1; j++) {
			printf("Opening device %d: %s\n", i*num_dev1 + j, argv[i*num_dev1 + j + 2]);
			
			dev_fd[i][j] = open(argv[i*num_dev1 + j + 2], O_RDWR);
			if(dev_fd[i][j] < 0){
				dev_fd[i][j]=-1;
			}
		}
	}
	
	// vars for parsing input line
	char operation[20];
	int sector;
	int count;
	int currWork;
	char buf[1024 * 1024];
	int c = 0;
	int reDevice = -1;
	int sizeRead = 1024 * 1024;
	// read input lines to get command of type "OP <SECTOR> <COUNT>"
	while (fgets(line, 1024, stdin) != NULL) {
		if (sscanf(line, "%s %d %s", operation, &sector, rep) != 3) {
			printf("Not enough arguments, try again.\n");
			continue;
		}
		
		// KILL specified device
		if (!strcmp(operation, "KILL")) {
			close(dev_fd[sector / num_dev0][sector % num_dev1]);
			dev_fd[sector / num_dev0][sector % num_dev1] = -1;
		}
		else if (!strcmp(operation, "REPAIR")) {
			close(dev_fd[sector / num_dev0][sector % num_dev1]);
			
			dev_fd[sector / num_dev0][sector % num_dev1] = -1;
			if ((reDevice = open(rep, O_RDWR)) < 0)
				continue;
			if (reDevice >= 0) {
				while (1) {
					currWork = findWorkingDevice(sector / num_dev0);
					if (currWork == -1) {
						printf("No working devices, can't copy\n");
						dev_fd[sector / num_dev0][sector % num_dev1] = reDevice;
						break;
					}
					if (lseek(dev_fd[sector / num_dev0][currWork], c, SEEK_SET) == -1)
						continue;
					if ((sizeRead = read(dev_fd[sector / num_dev0][currWork], buf, 1024 * 1024)) != -1) {
						
						c += sizeRead;
						
						
					}
					if (c == DEVICE_SIZE) {//Done copying
						dev_fd[sector / num_dev0][sector % num_dev1] = reDevice;
						
						break;
					}
				}
			}
		}
		// READ / WRITE
		else {
			count = atoi(rep);
			do_raid0_rw(operation, sector, count);
		}
	}
	closeAll();
}
