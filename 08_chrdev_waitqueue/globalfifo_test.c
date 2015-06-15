#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#define LENGTH	0x1000

int main(int argc, char *argv[])
{
	int fd;
	char str[LENGTH];
	int len = 0;

	if(argc < 2)
	{
		printf("Usage %s arg...\n", argv[0]);
		return -1;
	}

	fd = open(argv[1], O_RDWR);

	if(!fd)
	{
		printf("globalfifo node not exit.\n");
		return -2;
	}

	if(!strcmp(argv[2], "-r"))
	{
		len = read(fd, str, LENGTH);
		printf("Read:\n%s", str);
	}
	else if(!strcmp(argv[2], "-w"))
	{
		if(argc != 4)
		{
			printf("Usage %s -w text\n", argv[0]);			
		}
		else
		{
			len = write(fd, argv[3], strlen(argv[3]));
		}
	}
	else if(!strcmp(argv[2], "-c"))
	{
		ioctl(fd, 0x01);
	}

	close(fd);

	return 0;
}

