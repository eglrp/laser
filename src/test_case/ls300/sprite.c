
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>


#define FIFO "/tmp/test.sprite"
//#define WIDTH 360
//#define HEIGHT 180
char buf_r[4096];


static void printfv(char* buf, int len) {
	char c, l = '0';
	int count = 0;

	while (len--) {
		c = *buf++;
		if (c == l) {
			count++;
		} else {
			if (count > 0)
				printf(" %c[%d]", l == 0 ? '0' : l, count);
			l = c;
			count = 1;
		}
	}
	if (count > 0)
		printf(" %c[%d]", l == 0 ? '0' : l, count);
	printf("\n");
}

//本程序从一个FIFO读数据，并把读到的数据打印到标准输出
//如果读到字符“Q”，则退出
int main(int argc, char** argv)
{
	int fd;
	int readed;
	unsigned int w,h;
	int nread;
	if((mkfifo(FIFO,0777) < 0) && (errno != EEXIST))
	{
		printf("不能创建FIFO\n");
		exit(1);
	}

	printf("准备读取数据\n");
	fd = open(FIFO, O_RDONLY, 0);
	if(fd == -1)
	{
		perror("打开FIFO");
		exit(1);
	}

	nread = read(fd, buf_r, 18);

	if(nread != 18 || strncmp(buf_r,"ABCD",4)) {
		printf("接收宽高失败\n");
		exit(1);
	}

	sscanf(buf_r,"ABCD%05u%05uEFGH",&w,&h);
	printf("宽%u,高%u\n",w,h);

	for(;;)
	{
		nread = read(fd, buf_r, w);
		if(nread == -1)
		{
			if(errno == EAGAIN) {
				printf("没有数据,重试.\n");
				sleep(1);
				continue;
			}else{
				printf("管道关闭\n");
				break;
			}
		}else if(nread == 0){
			printf("管道关闭\n");
				break;
		}else if(nread != w)
		{
			printf("not enough data?\n");
			while(nread<w)
				nread += read(fd, buf_r+nread, w-nread);
		}

		//假设取到Q的时候退出
		if(buf_r[0]=='Q') break;
		printf("从FIFO读取的数据[%d]为：\t",nread);
		printfv(buf_r,w);
	}

}
