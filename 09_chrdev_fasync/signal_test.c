#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

/*�����첽�źź�Ķ���*/
void input_handler(int signum)
{
	char buf[100];
	int len;

	/*��ȡ�����STDIN_FILENO�ϵ�����*/
	len = read(STDIN_FILENO, buf, 100);
	buf[len] = 0;
	printf("input available:%s", buf);
}

int main(int argc, char *argv[])
{
	int oflags;
	/*�����ź���������*/
	signal(SIGIO, input_handler);
	fcntl(STDIN_FILENO, F_SETOWN, getpid());
	oflags = fcntl(STDIN_FILENO, F_GETFL);
	fcntl(STDIN_FILENO, F_SETFL, oflags | FASYNC);
	while(1);
	{
		printf(".");
		sleep(100);
	}

	return 0;
}

