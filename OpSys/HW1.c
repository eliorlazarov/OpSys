#include <stdio.h>
//#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h> // for open flags
#include <time.h> // for time measurement
#include <assert.h>
#include <errno.h> 
#include <string.h>


void encryptDecrypt(const wchar_t* toEncrypt, int length,char* key, int keyLen)
{
//	const wchar_t* key = L"KEY"; // A readonly wide String
//	wchar_t* output = new wchar_t[length];  // Make a temporary buffer

	for (int i = 0; i < length; i++)
	{
		output[i] = toEncrypt[i] ^ key[i % wcslen(key)];    // i % 3 wil be ascending between 0,1,2
	}
	return output;
}

int main(int argc, char* argv[])
{
	char temp;
	char currKey;
	if (argc != 4){
		printf("Not enough argumaents/n");
		return 1;
	}
	char buf[1];
	int fd = open(argv[1]);
	if (fd < 0){
		printf("Error opening file: %s\n", strerror(errno));
		return errno;
	}
	int fdKey = open(argv[2]);
	if (fdKey < 0){
		printf("Error opening file: %s\n", strerror(errno));
		return errno;
	}
	while (read(fd, buf, 1) == 1){
		currKey
		write();
	}
	if (wrtie(fd,buf,1)<1)
}