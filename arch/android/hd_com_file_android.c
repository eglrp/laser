//----------------------------------------------------------------------------
//	hd_com_file_android.c
//	kevin.ban
// 	2008-01-20
//
//
//  port for andorid
//	uplusplus 2010-6-25
//	yjcpu@163.com
//----------------------------------------------------------------------------

#ifdef ANDROID_OS
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#include <hd_com_api.h>
#include <hd_event_api.h>
#include <hd_thread_api.h>

/*----------------------------------------------------------------------------
termios 函数族提供了一个常规的终端接口，用于控制非同步通信端口。 这个结构包含了至少下列成员：
tcflag_t c_iflag;       输入模式
tcflag_t c_oflag;       输出模式
tcflag_t c_cflag;       控制模式
tcflag_t c_lflag;       本地模式
cc_t c_cc[NCCS];        控制字符
struct termios
{unsigned short c_iflag;  输入模式标志
unsigned short c_oflag;  输出模式标志
unsigned short c_cflag;  控制模式标志
unsigned short c_lflag; 区域模式标志或本地模式标志或局部模式
unsigned char c_line;   行控制line discipline
unsigned char c_cc[NCC]; 控制字符特性
};
**********************************************************************************************
c_iflag为16位的整型，控制输入属性的标志集，它每一位对应的掩码如下：

IGNBRK：忽略输入中断条件；
BRKINT：Signal interrupt on break(不懂)
IGNPAR：忽略帧错误和奇偶校验错误
PARMMRK：标注奇偶错误
INPCK：开启奇偶校验
ISTRIP：舍弃第8个字节
INLCR:将NL（\n）转化成CR(\r)
IGNCR：忽略CR
ICRNL：将CR(\r)转化成NL(\n)
IUCLC：大写字母转化成小写字母
IXON： 开启输出 XON/XOFF控制
IMAXBEL：输入列队满时报警
IUTF8：默认输入编码为UTF8
**********************************************************************************************
c_oflag为16位的整型，控制输出属性的标志集，它每一位对应的掩码如下

OPOST:开启
OLCUC：将小写字母转化成大写字母
ONLCR:\n转化成\r
OCRNL:\r转化成\n
ONOCR:第一行不输出\r
ONLRET:不输出\r
OFILL:输出默认填充字符作为延时
OFDEL:设置空白字符为ASCII DEL（0177），如果不设置空白字符为ASCII NUL('\0')
NLDLY:Newline delay mask
CRDLY：Carriage return delay mask
TABLY:Horizontal tab delay mask
BSDLY:Backspace delay mask
VTDLY:Form feed delay mask
*********************************************************************************************
c_cflag为控制模式的设置：
CBAUD：波特速度掩码
CBAUDEX：Extra baud speed mask
CSIZE:字符大小掩码
CSTOPB：设置2位的停止字节
CREAD：开启receiver
PARENB：开启对输出产生奇偶校验码，对输入进行奇偶校验
PAROOD：奇偶校验使用奇数
HUPCL：Lower modem control lines after last process closes the device
CLOCAL:忽略调制控制行
LOBLK:输入速度地掩码
CMSPAR：use"stick"(mark/space)parity
CRTSCTS:开启RTS/CTS流控制
**********************************************************************************************
c_lflag为本地模式设置：

ISIG：当接受到INTR，QUIT，SUSP，DSUSP字符，产生响应信号
ICANON：开启canonical模式
XCASE：输入都为小写字母，输出都为大写字母
ECHO：显示回显位的状态
*/
//static method
int speed_arr[] = { B38400, B19200, B9600, B4800, B2400, B1200, B300, B38400,
		B19200, B9600, B4800, B2400, B1200, B300, };
int name_arr[] = { 38400, 19200, 9600, 4800, 2400, 1200, 300, 38400, 19200,
		9600, 4800, 2400, 1200, 300, };
static int set_speed(int fd, int speed) {
	int i;
	int status;
	struct termios Opt;
	tcgetattr(fd, &Opt);
	for (i = 0; i < sizeof(speed_arr) / sizeof(int); i++) {
		if (speed == name_arr[i]) {
			tcflush(fd, TCIOFLUSH);
			cfsetispeed(&Opt, speed_arr[i]);
			cfsetospeed(&Opt, speed_arr[i]);
			status = tcsetattr(fd, TCSANOW, &Opt);
			if (status != 0) {
				DMSG((STDOUT, "tcsetattr fd"));
				return 0;
			}
			tcflush(fd, TCIOFLUSH);
			return 1;
		}
	}
	return 0;
}
/**
 *@brief   设置串口数据位，停止位和效验位
 *@param  fd     类型  int  打开的串口文件句柄
 *@param  databits 类型  int 数据位   取值 为 7 或者8
 *@param  stopbits 类型  int 停止位   取值为 1 或者2
 *@param  parity  类型  int  效验类型 取值为N,E,O,S
 */
static int set_parity(int fd, int databits, int stopbits, int parity) {
	struct termios options;
	if (tcgetattr(fd, &options) != 0) {
		perror("SetupSerial 1");
		return (FALSE);
	}
	options.c_cflag &= ~CSIZE;
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); /*Input*/
	options.c_oflag &= ~OPOST; /*Output*/

	switch (databits) /*设置数据位数*/
	{
	case 7:
		options.c_cflag |= CS7;
		break;
	case 8:
		options.c_cflag |= CS8;
		break;
	default:
		DMSG((STDOUT, "Unsupported data size\n"));
		return (FALSE);
	}
	switch (parity) {
	case 'n':
	case 'N':
		options.c_cflag &= ~PARENB; /* Clear parity enable */
		options.c_iflag &= ~INPCK; /* Enable parity checking */
		break;
	case 'o':
	case 'O':
		options.c_cflag |= (PARODD | PARENB); /* 设置为奇效验*/
		options.c_iflag |= INPCK; /* Disnable parity checking */
		break;
	case 'e':
	case 'E':
		options.c_cflag |= PARENB; /* Enable parity */
		options.c_cflag &= ~PARODD; /* 转换为偶效验*/
		options.c_iflag |= INPCK; /* Disnable parity checking */
		break;
	case 'S':
	case 's': /*as no parity*/
		options.c_cflag &= ~PARENB;
		options.c_cflag &= ~CSTOPB;
		break;
	default:
		DMSG((STDOUT, "Unsupported parity\n"));
		return (FALSE);
	}
	/* 设置停止位*/
	switch (stopbits) {
	case 1:
		options.c_cflag &= ~CSTOPB;
		break;
	case 2:
		options.c_cflag |= CSTOPB;
		break;
	default:
		DMSG((STDOUT, "Unsupported stop bits\n"));
		return (FALSE);
	}
	/* Set input parity option */
	if (parity != 'n')
		options.c_iflag |= INPCK;
	tcflush(fd, TCIFLUSH);
	options.c_cc[VTIME] = 0; /* 设置超时0 seconds*/
	options.c_cc[VMIN] = 13; /* define the minimum bytes data to be readed*/
	if (tcsetattr(fd, TCSANOW, &options) != 0) {
		perror("SetupSerial 3");
		return (FALSE);
	}
	return (TRUE);
}
//----------------------------------------------------------------------------
e_uint8 Comm_Open(com_t *com, const char* com_dev, const int bandrate) {
	int fd = open(com_dev, O_RDWR);
	//| O_NOCTTY | O_NDELAY
	if (-1 == fd) {
		DMSG((STDOUT, "Can't Open Serial Port\r\n"));
		return 0;
	}
	if (!set_speed(fd, bandrate)) {
		DMSG((STDOUT, "faild to set speed \n"));
		return 0;
	}
	if (!set_parity(fd, 8, 1, 'N')) {
		DMSG((STDOUT, "faild to set parity \n"));
		return 0;
	}
	strncpy(com->name, com_dev, sizeof(com->name));
	com->priv = (void*) fd;
	com->state = TRUE;
	com->ready = TRUE;
	return 1;
}

void Comm_Close(com_t *com) {
	int fd;
	if (Comm_State(com)) {
		fd = (int) com->priv;
		if (fd > 0)
			close(fd);
	}
	BZERO(com, com_t);
	return;
}

e_uint8 Comm_State(com_t *com) {
	return (com ? com->state : 0);
}

//	attach and detach thread object
ethread_t *
Comm_Attach(com_t *com, ethread_t *thread) {
	ethread_t *old;
	old = com->thread;
	com->thread = thread;
	return old;
}

void Comm_Detach(com_t *com) {
	if (!Comm_State(com))
		return;
	com->thread = 0x0;
}

//	get/set comm state
e_bool Comm_GetData(com_t *com, com_dcb_t *dcb) {
	if (!Comm_State(com) || !dcb)
		return FALSE;
	return TRUE;
}

e_bool Comm_SetData(com_t *com, const com_dcb_t *dcb) {
	if (!Comm_State(com) || !dcb)
		return 0;
	com->dcb = *dcb;
	return 1;

}

//	get/set comm monitored mask
e_uint32 Comm_SetMask(com_t *com, e_uint32 mask) {
	return 1;
}

e_uint32 Comm_GetMask(com_t *com) {
	return 1;
}

//	clear comm output/input
e_bool Comm_Purge(com_t *com, e_uint32 flag) {
	int fd;
	if (!Comm_State(com))
		return 0;
	fd = (int) com->priv;
	tcflush(fd, TCIFLUSH);
	return 1;
}

e_uint32 Comm_Read(com_t *com, e_uint8 *buffer, e_uint32 blen) {
	int fd;
	if (!Comm_State(com))
		return 0;
	fd = (int) com->priv;
	int ret = read(fd, buffer, blen);
	if (ret < 0)
		return E_ERROR;
	return ret;
}

//	write comm data
e_uint32 Comm_Write(com_t *com, unsigned char *buffer, unsigned long blen) {
	int fd;
	if (!Comm_State(com))
		return 0;
	fd = (int) com->priv;
	int ret = write(fd, buffer, blen);
	if (ret < 0)
		return E_ERROR;
	return ret;
}

#endif	/*ANDROID_OS*/
//----------------------------------------------------------------------------
//

