#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
	int fd;
	unsigned int count = 0;
	unsigned int old_count = 0;

	fd = open("/dev/second_dev0", O_RDONLY);
	if(fd != -1)
	{
		while(1)
		{
			read(fd, &count, sizeof(unsigned int));
			if(count != old_count)
			{
				printf("\rsecond:\t%d", count);
				fflush(stdout);
				old_count = count;
			}
		}
	}
	else
	{
		printf("open file failed\n");
	}

	close(fd);

	return 0;
}
