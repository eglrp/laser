#ifdef ANDROID_OS

#include <hd_serial_api.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>

static char serials[30][16] =
{ "/dev/ttyS0", "/dev/ttyS1", "/dev/ttyS2", "/dev/ttyS3", "/dev/ttyS4",
		"/dev/ttyS5", "/dev/ttyS6", "/dev/ttyS7", "/dev/ttyS8", "/dev/ttyS9",
		"/dev/ttyS10", "/dev/ttyS11", "/dev/ttyS12", "/dev/ttyS13",
		"/dev/ttyS14", "/dev/ttyS15", "/dev/ttyUSB0", "/dev/ttyUSB1",
		"/dev/ttyUSB2", "/dev/ttyUSB3", "/dev/ttyUSB4", "/dev/ttyUSB5",
		"/dev/ttyAMA0", "/dev/ttyAMA1", "/dev/ttyACM0", "/dev/ttyACM1",
		"/dev/rfcomm0", "/dev/rfcomm1", "/dev/ircomm0", "/dev/ircomm1" };

struct serial_handle_t
{
	e_int32 port_handle;
	struct termios options;
};

typedef struct
{
	speed_t baudOut;
	e_int32 baudIn;
} baudSelect_t;

static const baudSelect_t baudSelect[] =
{
{ B50, 50 },
{ B75, 75 },
{ B110, 110 },
{ B134, 134 },
{ B150, 150 },
{ B200, 200 },
{ B300, 300 },
{ B600, 600 },
{ B1200, 1200 },
{ B1800, 1800 },
{ B2400, 2400 },
{ B4800, 4800 },
{ B9600, 9600 },
{ B19200, 19200 },
{ B38400, 38400 },
{ B57600, 57600 },
{ B115200, 115200 } };

e_int32 Serial_Open(serial_t *port, char *name)
{
	e_int32 fd;
	e_assert(port, E_ERROR_INVALID_PARAMETER);

	port->priv = (struct serial_handle_t*) malloc(
			sizeof(struct serial_handle_t));
	e_assert(port->priv, E_ERROR_BAD_ALLOCATE);

	/* open port */
	fd = open(name, O_RDWR | O_NOCTTY);

	/* failed to open */
	if (fd == -1)
	{
		port->priv->port_handle = 0;
		return E_ERROR;
	}

	/* get current options */
	tcgetattr(port->priv->port_handle, &(port->priv->options));

	/* return handle to port */
	port->priv->port_handle = fd;
	strncpy(port->name, name, sizeof(port->name));
	port->state = TRUE;

	return E_OK;
}

e_int32 Serial_Close(serial_t *port)
{
	e_assert(port&&port->state, E_ERROR_INVALID_PARAMETER);

	if (port->priv)
	{
		/* restore the options to their original state */
		tcsetattr(port->priv->port_handle, TCSANOW, &(port->priv->options));

		/* close the port */
		if (close(port->priv->port_handle) == -1)
		{
			return E_ERROR;
		}
		free(port->priv);
	}
	port->state = 0;
	return E_OK;
}

e_int32 Serial_Settings(serial_t *port, e_uint32 baud, char parity,
		e_uint8 dataBits, e_uint8 stopBits, e_int32 timeout_tenths)
{
	static struct termios options;
	speed_t actual_baud;
	e_int32 numBaud;
	e_int32 i;

	e_assert(port&&port->state, E_ERROR_INVALID_PARAMETER);

	/* clear termios struct */
	memset(&options, 0, sizeof(options));

	port->speed = baud;
	/* check baud is valid */
	numBaud = sizeof(baudSelect) / sizeof(baudSelect_t);
	for (i = 0; i < numBaud; i++)
	{
		if (baud == baudSelect[i].baudIn)
		{
			actual_baud = baudSelect[i].baudOut;
			break;
		}
	}
	if (i == numBaud)
	{
		return E_ERROR_INVALID_PARAMETER;
	}

	/* set port speed */
	cfsetispeed(&options, baud);
	cfsetospeed(&options, baud);

	/* check and set parity */
	switch (parity)
	{
	case 'n':
		options.c_cflag &= ~PARENB;
		break;

	case 'o':
		options.c_cflag |= PARENB;
		options.c_cflag |= PARODD;
		break;

	case 'e':
		options.c_cflag |= PARENB;
		options.c_cflag &= ~PARODD;
		break;

	default:
		return E_ERROR_INVALID_PARAMETER;
	}

	/* check and set data bits */
	options.c_cflag &= ~CSIZE;
	switch (dataBits)
	{
	case 5:
		options.c_cflag |= CS5;
		break;

	case 6:
		options.c_cflag |= CS6;
		break;

	case 7:
		options.c_cflag |= CS7;
		break;

	case 8:
		options.c_cflag |= CS8;
		break;

	default:
		return E_ERROR_INVALID_PARAMETER;
	}

	/* check and set stop bits */
	switch (stopBits)
	{
	case 1:
		options.c_cflag &= ~CSTOPB;
		break;

	case 2:
		options.c_cflag |= CSTOPB;
		break;

	default:
		return E_ERROR_INVALID_PARAMETER;
	}

	/* set timeouts */
	options.c_cc[VMIN] = 0;
	options.c_cc[VTIME] = timeout_tenths;

	options.c_cflag |= CLOCAL | CREAD;

	/* set options for port */
	if (tcsetattr(port->priv->port_handle, TCSAFLUSH, &options) != 0)
	{
		return E_ERROR;
	}

	return E_OK;
}

e_int32 Serial_Timeouts(serial_t *port, int readTimeout_usec,
		int writeTimeout_usec)
{
	e_assert(port&&port->state, E_ERROR_INVALID_PARAMETER);
	port->read_timeout_usec = readTimeout_usec;
	port->write_timeout_usec = writeTimeout_usec;
	return E_OK;
}

e_int32 Serial_Select(serial_t *port, e_int32 type, e_int32 timeout_usec)
{
	int fd, ret;
	fd_set inputs, checkfds;
	struct timeval timeout;

	e_assert(port&&port->state, E_ERROR);

	fd = port->priv->port_handle;
	//用select函数之前先把集合清零
	FD_ZERO(&inputs);

	//把要检测的句柄fd，加入到集合里。
	FD_SET(fd, &inputs);

	while (1)
	{
		checkfds = inputs;
		/*如果参数timeout设为NULL则表示select（）没有timeout
		 执行成功则返回文件描述词状态已改变的个数，如果返回0代表在描述词状态改变前已超过timeout时间，
		 当有错误发生时则返回-1，错误原因存于errno，此时参数readfds，writefds，exceptfds和
		 timeout的值变成不可预测。
		 EBADF 文件描述词为无效的或该文件已关闭
		 EINTR 此调用被信号所中断
		 EINVAL 参数n 为负值。
		 ENOMEM 核心内存不足
		 */
		switch (type)
		{
		case E_READ:
			//DMSG(( STDOUT,"Com_Select FD=%d timeout=%dus",fd, timeout_usec));
			if (timeout_usec > 0)
			{
				timeout.tv_sec = (long) (timeout_usec / 1000000);
				timeout.tv_usec = (long) (timeout_usec % 1000000);
				ret = select(fd + 1, &checkfds, (fd_set *) 0, (fd_set *) 0,
						&timeout);
			}
			else
			{
				ret = select(fd + 1, &checkfds, (fd_set *) 0, (fd_set *) 0,
						NULL);
			}
			//DMSG((STDOUT,"Com_Select %d socket can read/write",ret));
			break;
		case E_WRITE:
			if (timeout_usec > 0)
			{
				timeout.tv_sec = (long) (timeout_usec / 1000000);
				timeout.tv_usec = (long) (timeout_usec % 1000000);
				ret = select(fd + 1, (fd_set *) 0, &checkfds, (fd_set *) 0,
						&timeout);
			}
			else
			{
				ret = select(fd + 1, (fd_set *) 0, &checkfds, (fd_set *) 0,
						NULL);
			}
			break;
		default:
			return E_ERROR_INVALID_PARAMETER;
		}

		if (e_check(ret==0,"Socket select time out."))
		{
			return E_ERROR_TIME_OUT;
		}
		if (e_check(ret==-1,"Socket select error."))
		{
			return E_ERROR_IO;
		}

		switch (type)
		{
		case E_READ:
			ret = FD_ISSET(fd, &checkfds);
			e_assert(ret, E_ERROR_IO);
			ioctl(fd, FIONREAD, &ret); //取得数据长度
			e_assert((ret > 0), E_ERROR_IO);
			//DMSG((STDOUT,"Com_Select %d data can read",ret));
			//可读数据长度>0
			return ret;
		case E_WRITE:
			ret = FD_ISSET(fd, &checkfds);
			e_assert(ret, E_ERROR_IO);
			//DMSG((STDOUT,"Com_Select socket ready to write"));
			return E_OK;
		}

	}
	return E_ERROR;
}

e_int32 Serial_Read(serial_t *port, e_uint8 *data, e_int32 size)
{
	e_int32 readCount = 0, bytesRead = 0;

	e_assert(port&&port->state, E_ERROR);

	bytesRead = 0;
	//简单实现全局读超时设置，待优化
	if (port->read_timeout_usec > 0)
	{
		Serial_Select(port, E_READ, port->read_timeout_usec);
	}

	/* loop reading bytes from port until all bytes are read or there is a timeout*/
	while (bytesRead != size)
	{
		/* get bytes from port */
		readCount = read(port->priv->port_handle, data + bytesRead,
				size - bytesRead);
		bytesRead += readCount;

		/* if no bytes read timeout has occured */
		if (readCount == 0)
		{
			break;
		}
	}

	return bytesRead;
}

e_int32 Serial_Write(serial_t *port, e_uint8 *data, e_int32 size)
{
	e_int32 bytesWritten = 0, writeCount = 0;
	e_assert(port&&port->state, E_ERROR);
	//简单实现全局写超时设置，待优化
	if (port->write_timeout_usec > 0)
	{
		Serial_Select(port, E_WRITE, port->write_timeout_usec);
	}

	/* loop reading bytes from port until all bytes are read or there is a timeout*/
	while (bytesWritten != size)
	{
		/* get bytes from port */
		writeCount = write(port->priv->port_handle, data + bytesWritten,
				size - bytesWritten);
		bytesWritten += writeCount;

		/* if no bytes read timeout has occured */
		if (writeCount == 0)
		{
			break;
		}
	}

	return bytesWritten;
}

e_int32 SerialEnumerate(void)
{
	return E_ERROR;
}

#endif /*ANDROID_OS*/
