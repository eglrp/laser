
#include <arch/hd_pipe_api.h>

#ifdef ANDROID_OS

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

struct pipe_handle_t
{
	int pip_handle;
};

typedef int pipe_handle_t;

e_int32 Pipe_Open(pipe_t *pip, char *name, int size, int mode)
{
	e_int32 fd, ret;
	e_assert(pip&&name&&size, E_ERROR_INVALID_PARAMETER);

	if (mode == E_WRITE) {
		if (access(name, F_OK) == -1)
				{
			ret = mkfifo(name, 0777);
			if (ret != 0)
					{
				DMSG((STDOUT, "Could not create fifo %s\n", name));
				return E_ERROR;
			}
		}
		fd = open(name, O_WRONLY | O_NONBLOCK, 0);
		/*
		 * If O_NONBLOCK is set:
		 An open() for reading only will return without delay.
		 An open() for writing only will return an error if
		 no process currently has the file open for reading.
		 * If O_NONBLOCK is clear:
		 An open() for reading only will block the calling thread
		 until a thread opens the file for writing.
		 An open() for writing only will block the calling
		 thread until a thread opens the file for reading.
		 * */
	}
	else {
		fd = open(name, O_RDONLY | O_NONBLOCK, 0);
		if (fd == -1)
				{
			DMSG((STDOUT,"open pipe failed\n"));
			return E_ERROR;
		}
	}

	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK) ; //设置为非阻塞模式
	//分配内存
	pip->priv = (struct pipe_handle_t*) malloc(
												sizeof(struct pipe_handle_t));
	e_assert(pip->priv, E_ERROR_BAD_ALLOCATE);
	/* save handle to pip */
	pip->priv->pip_handle = fd;
	pip->mode = mode;
	strncpy(pip->name, name, sizeof(pip->name));
	pip->state = TRUE;
	return E_OK;
}

e_int32 Pipe_Close(pipe_t *pip)
{
	e_assert(pip&&pip->state, E_ERROR_INVALID_PARAMETER);

	if (pip->priv)
	{
		/* close the pip */
		if (close(pip->priv->pip_handle) == -1)
				{
//			return E_ERROR;
			DMSG((STDOUT,"close Pipe error:%d\n",errno));
		}
		free(pip->priv);
	}
	pip->state = 0;
	return E_OK;
}

e_int32 Pipe_Timeouts(pipe_t *pip, int readTimeout_usec,
		int writeTimeout_usec)
{
	e_assert(pip&&pip->state, E_ERROR_INVALID_PARAMETER);
	pip->read_timeout_usec = readTimeout_usec;
	pip->write_timeout_usec = writeTimeout_usec;
	return E_OK;
}

e_int32 Pipe_Select(pipe_t *pip, e_int32 type, e_int32 timeout_usec)
{
	int fd, ret;
	fd_set inputs, checkfds;
	struct timeval timeout;

	e_assert(pip&&pip->state, E_ERROR);

	fd = pip->priv->pip_handle;

	if (fd == -1) {
		fd = open(pip->name, O_WRONLY | O_NONBLOCK, 0);
		if (fd == -1)
			return E_ERROR;
		pip->priv->pip_handle = fd;
	}

	//用select函数之前先把集合清零
	FD_ZERO(&inputs);

	//把要检测的句柄fd，加入到集合里。
	FD_SET(fd, &inputs);

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
			timeout.tv_sec = (long) (timeout_usec / ((1000 * 1000)));
			timeout.tv_usec = (long) (timeout_usec % ((1000 * 1000)));
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
			timeout.tv_sec = (long) (timeout_usec / ((1000 * 1000)));
			timeout.tv_usec = (long) (timeout_usec % ((1000 * 1000)));
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

	//if (e_check(ret==0,"Pipe select time out."))
	if (ret == 0)
			{
		return E_ERROR_TIME_OUT;
	}
	if (e_check(ret==-1,"Pipe select error."))
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

	return E_ERROR;
}

e_int32 Pipe_Read(pipe_t *pip, e_uint8 *data, e_int32 size)
{
	e_int32 readCount = 0, bytesRead = 0;

	e_assert(pip&&pip->state, E_ERROR);

	bytesRead = 0;
	//简单实现全局读超时设置，待优化
	if (pip->read_timeout_usec > 0)
			{
		Pipe_Select(pip, E_READ, pip->read_timeout_usec);
	}

	/* loop reading bytes from pip until all bytes are read or there is a timeout*/
	while (bytesRead != size)
	{
		/* get bytes from pip */
		readCount = read(pip->priv->pip_handle, data + bytesRead,
							size - bytesRead);
		if (readCount == -1) {
			if (errno == EAGAIN) {
				DMSG((STDOUT,"No data yet,try later\n"));
				Delay(1);
				continue;
			}
		}
		if(readCount ==0) //write pipe is closed
			continue;

		bytesRead += readCount;

		/* if no bytes read timeout has occured */
		if (readCount == 0)
				{
			break;
		}
	}

	return bytesRead;
}

e_int32 Pipe_Write(pipe_t *pip, e_uint8 *data, e_int32 size)
{
	e_int32 bytesWritten = 0, writeCount = 0;
	e_assert(pip&&pip->state, E_ERROR);
	//简单实现全局写超时设置，待优化
	if (pip->write_timeout_usec > 0)
			{
		Pipe_Select(pip, E_WRITE, pip->write_timeout_usec);
	}

	/* loop writing bytes to pip until all bytes are write or there is a timeout*/
	while (bytesWritten != size)
	{
		/* write bytes to pip */
		writeCount = write(pip->priv->pip_handle, data + bytesWritten,
							size - bytesWritten);

		if (writeCount == -1) {
			if (errno == EAGAIN) {
				//DMSG((STDOUT,"Data not read yet,try later\n"));
				Delay(1);
				continue;
			}
		}

		bytesWritten += writeCount;

		/* if no bytes read timeout has occured */
		if (writeCount == 0)
				{
			break;
		}
	}

	return bytesWritten;
}

#endif /*ANDROID_OS*/
