#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

/*接收异步信号后的动作*/
void input_handler(int signum)
{
	printf("receive a signal from globalfifo, signalnum:%d\n", signum);
}

int main(int argc, char *argv[])
{
	int fd, oflags;
	fd = open("/dev/globalfifo0", O_RDWR, S_IRUSR | S_IWUSR);
	if(fd != -1)
	{
		/*启动信号驱动机制*/		
		signal(SIGIO, input_handler);
		fcntl(fd, F_SETOWN, getpid());
		oflags = fcntl(fd, F_GETFL);
		fcntl(fd, F_SETFL, oflags | FASYNC);
		while(1);
		{
			printf(".");
			sleep(100);
		}
	}
	else
	{
		printf("device open failure\n");
	}

	return 0;
}
