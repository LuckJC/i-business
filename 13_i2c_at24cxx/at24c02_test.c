#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
	int fd, ret, address, count;
	char buf[2048];

	if(argc < 2)
		goto err;

	fd = open("/dev/at24cxx0", O_RDWR);
	if(fd == -1)
		goto err;

	if(argv[1][0] == 'r')
	{
		printf("address:");
		scanf("%x", &address);
		lseek(fd, address, 0);
		printf("count:");
		scanf("%d", &count);		
		ret = read(fd, buf, count);
		if(ret == 0)
			goto err;
		printf("read:\n%s\n", buf);
	}
	else if(argv[1][0] == 'w')
	{
		printf("address:");
		scanf("%x", &address);
		lseek(fd, address, 0);
		printf("data:");
		getchar();
		gets(buf);
		//scanf("%s", buf);
		ret = write(fd, buf, strlen(buf)+1);
		if(ret == 0 )
			goto err;
		printf("write OK!\n");
	}
	
	return 0;

err:
	printf("Error!\n");	
	return -1;
}
