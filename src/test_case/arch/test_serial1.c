//#include <stdio.h>
//#include <sys/ioctl.h>
//#include <fcntl.h>
//#include <termios.h>
//#include <stdlib.h>
//
///* These are the hash definitions */
//#define USERBAUD1200 '1'+'2'
//#define USERBAUD2400 '2'+'4'
//#define USERBAUD9600 '9'+'6'
//#define USERBAUD1920 '1'+'9'
//#define USERBAUD3840 '3'+'8'
//#define DEV "/dev/ttyS0"
//struct termios tio;
//
///* write the users command out the serial port */
//int send_cmd(int ftty, char * str) {
//	int result;
//	result = write(ftty, str, strlen(str));/*argv[4], strlen(argv[4]));*/
//	if (result < 0) {
//
//		printf("Write command:%s to VC312 failed/n", str);
//		close(ftty);
//		exit(1);
//	}
//}
//
//int init_dev(struct termios *oldtio) {
//	int fd, status;
//	unsigned char cmd[17];
//	struct termios newtio;
//
//	//open carmo device
//	fd = open(DEV, O_RDWR | O_NOCTTY | O_NDELAY);
//	if (fd == -1) {
//		perror("open"DEV);
//		return -1;
//	} else { //set to block;
//		fcntl(fd, F_SETFL, 0);
//	}
//
//	printf("open carmo ok/n");
//
//	tcgetattr(fd, oldtio); //save current serial port settings
//
//	bzero(&newtio, sizeof(newtio)); // clear struct for new port settings
//	//configure the serial port;
//	cfsetispeed(&newtio, B115200);
//	cfsetospeed(&newtio, B115200);
//	newtio.c_cflag |= CLOCAL | CREAD;
//	/*8N1*/
//	newtio.c_cflag &= ~CSIZE; /* Mask the character size bits */
//	newtio.c_cflag |= CS8; /* Select 8 data bits */
//	newtio.c_cflag &= ~PARENB;
//	newtio.c_cflag &= ~CSTOPB;
//	newtio.c_cflag &= ~CRTSCTS; //disable hardware flow control;
//	newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);/*raw input*/
//	newtio.c_oflag &= ~OPOST; /*raw output*/
//	tcflush(fd, TCIFLUSH); //clear input buffer
//	newtio.c_cc[VTIME] = 100; /* inter-character timer unused */
//	newtio.c_cc[VMIN] = 0; /* blocking read until 0 character arrives */
//	tcsetattr(fd, TCSANOW, &newtio);
//	/*
//	 ioctl(fd,TIOCMGET,&status);
//	 printf("the serial status is 0x%x/n",status);
//	 status |=TIOCM_RTS;
//	 ioctl(fd,TIOCMSET,&status);
//	 status=0;
//	 ioctl(fd,TIOCMGET,&status);
//	 printf("the serial status now is 0x%x/n",status);
//	 */
//
//	return fd;
//}
//int read_frame(int ftty, int streamfd) {
//	int n, max_fd, len, count = 0;
//	fd_set input;
//	struct timeval timeout;
//	char buffer[640 * 500 * 3];
//	char * ptr = buffer;
//
//	FD_ZERO(&input);
//	FD_SET(ftty, &input);
//	max_fd = ftty + 1;
//	/* Do the select */
//	while (1) {
//
//		/* Initialize the timeout structure */
//		timeout.tv_sec = 1;
//		timeout.tv_usec = 0;
//		n = select(max_fd, &input, NULL, NULL, &timeout);
//		if (n < 0)
//			perror("select failed");
//		else if (n == 0) {
//			puts("TIMEOUT");
//			break;
//		} else {
//			ioctl(ftty, FIONREAD, &len);
//			if (!len) {
//				fprintf(stderr, "Communication closed by server/n");
//				return -1;
//			}
//
//			len = read(ftty, ptr, len);
//			//ptr[len] = 0;
//			//printf("Readed %d data:%s/n",len,ptr);
//			write(streamfd, ptr, len);
//			ptr += len;
//			count += len;
//		}
//	}
//	buffer[count] = 0;
//	printf("Readed %d data:%s/n", count, buffer);
//	return 1;
//
//}
//
//int main(int argc, char *argv[]) {
//	int fd, count;
//	char buffer[255];
//	char cmd[30];
//	char filename[255];
//	int socket;
//	struct termios oldtio;
//// cmd[0]=0xff;cmd[1]=0xe1;cmd[2]=0x52;cmd[3]=0x52;cmd[4]=0x80;cmd[5]=0x80;cmd[6]=0;
//// if(send_cmd(fd,cmd)<1) exit(0);
//// usleep(400*1000);
//
//	fd = init_dev(&oldtio);
//	if (fd < 0)
//		exit(-1);
//	cmd[0] = 0xff;
//	cmd[1] = 0xe1;
//	cmd[2] = 0x58;
//	cmd[3] = 0x58;
//	cmd[4] = 0x66;
//	cmd[5] = 0x66;
//	cmd[6] = 0;
//	if (send_cmd(fd, cmd) < 1)
//		exit(0);
//
//	sprintf(filename, "1.jpg"); //"/tmp/get%d.jpg",i);
//	socket = open(filename, O_CREAT | O_RDWR);
//	read_frame(fd, socket);
//	close(socket);
//	tcsetattr(fd, TCSANOW, &oldtio);
//
//}
