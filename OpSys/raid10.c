// usage example:
// sudo ./raid0 /dev/sdb /dev/sdc

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h> // for open flags
#include <assert.h>
#include <errno.h> 
#include <string.h>

#define DEVICE_SIZE (1024*1024*256) // assume all devices identical in size

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
	for (int i = 0; i < num_dev1; i++) {
		if (dev_fd[dev0][i] >= 0)
			return i;
	}
	return -1;
}

void do_raid0_rw(char* operation, int sector, int count)
{
	int i = sector;

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
			exit(0);
		}
		// find offset of sector inside device
		int block_start = i / (num_dev0 * SECTORS_PER_BLOCK);
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
		assert(offset == lseek(dev_fd[dev_num], offset, SEEK_SET));

		if (!strcmp(operation, "READ")) {
			while (1) {
				if (size != read(dev_fd[dev_num][defaultDev], buf, size)) {
					close(dev_fd[dev_num][defaultDev]);
					dev_fd[dev_num][defaultDev] = -1;
					defaultDev = findWorkingDevice(dev_num);
					if (defaultDev == -1)
					{
						printf("No working drives\n");
						exit(0);
					}
				}
				else
					break;
			}
			

		}
		else if (!strcmp(operation, "WRITE"))
			for (int j = 0; j < num_dev1; j++)
				if (dev_fd[num_dev0][i] >= 0)
					if (size != write(dev_fd[num_dev0][j], buf, size)) {
						close(dev_fd[num_dev0][j]);
						dev_fd[num_dev0][j] = -1;
					}

		printf("Operation on device %d, sector %d-%d\n",
			dev_num, sector_start, sector_end);

		i += num_sectors;
	}
}
void closeAll() {
	for (int i = 0; i < num_dev0; ++i) {
		for (int j = 0; j < num_dev1; j++) {

			close(dev_fd[i][j]);
		}
	}
}

int main(int argc, char** argv)
{
	assert(argc > 2);

	int i;
	char line[1024];
	char rep[1024];
	// number of devices == number of arguments (ignore 1st)
	sscanf(argv[1], "%d", num_dev0);
	num_dev1 = (argc - 2) / num_dev0;
	int _dev_fd[num_dev0][num_dev1];
	dev_fd = _dev_fd;

	// open all devices
	for (i = 0; i < num_dev0; ++i) {
		for (int j = 0; j < num_dev1; j++) {
			printf("Opening device %d: %s\n", i, argv[i + 2]);
			dev_fd[i][j] = open(argv[i + j + 2], O_RDWR);
			assert(dev_fd[i][j] >= 0);
		}
	}

	// vars for parsing input line
	char operation[20];
	int sector;
	char countStr[1024];
	int count;
	// read input lines to get command of type "OP <SECTOR> <COUNT>"
	while (fgets(line, 1024, stdin) != NULL) {
		assert(sscanf(line, "%s %d %s", operation, &sector, &countStr) == 3);

		// KILL specified device
		if (!strcmp(operation, "KILL")) {
			assert(!close(dev_fd[sector / num_dev0][sector % num_dev1]));
			dev_fd[sector / num_dev0][sector % num_dev1] = -1;
		}
		else if (!strcmp(operation, "REPAIR")) {
			
			for (int i = 0; i < num_dev1; i++) {

			}
		}
		// READ / WRITE
		else {
			count = atoi(countStr);
			if (DEBUG)
				printf("%d\n", count);
			//do_raid0_rw(operation, sector, count);
		}
	}

	closeAll();
}