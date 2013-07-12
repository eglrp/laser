#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>

#include <arch/hd_pipe_api.h>
#include <arch/hd_thread_api.h>

#define MAX_ENTITY_NUM 10

struct pipe_handle_t
{
	void* buf1;
	void* buf2;
	void* read_buf;
	void* write_buf;
	int size;
	char name[MAX_PATH_LEN];
	semaphore_t read_sem;
	semaphore_t write_sem;
	int state;
	int ref;
};

struct pipe_list {
	struct pipe_handle_t handles[MAX_ENTITY_NUM];
	mutex_t mtx;
} pipes =
{
	.handles = { 0 },
	.mtx = { 0 }
};

static e_int32 assign_pipe_handle(pipe_t *pip, char* name, int size) {
	e_int32 ret = E_ERROR, i;
	if (!mutex_state(&pipes.mtx)) {
		ret = mutex_init(&pipes.mtx);
		e_assert(ret, E_ERROR);
	}

	ret = mutex_lock(&pipes.mtx);
	e_assert(ret, E_ERROR);

	for (i = 0; i < MAX_ENTITY_NUM; i++) { //find
		if (pipes.handles[i].state && !strncmp(pipes.handles[i].name, name, MAX_PATH_LEN)) {
			pip->priv = &pipes.handles[i];
			pipes.handles[i].ref++;
			ret = E_OK;
			goto end;
		}
	}

	for (i = 0; i < MAX_ENTITY_NUM; i++) { //create
		if (!pipes.handles[i].state) {
			strncpy(pipes.handles[i].name, name, MAX_PATH_LEN);

			pipes.handles[i].size = size;
			pipes.handles[i].buf1 = malloc(size * 2);
			pipes.handles[i].buf2 = pipes.handles[i].buf1 + size;
			pipes.handles[i].read_buf = pipes.handles[i].write_buf = pipes.handles[i].buf1;

			pip->priv = &pipes.handles[i];
			semaphore_init(&pipes.handles[i].read_sem, 0);
			semaphore_init(&pipes.handles[i].write_sem, 1);
			pipes.handles[i].state = 1;
			ret = E_OK;
			break;
		}
	}

	end:
	ret = mutex_unlock(&pipes.mtx);
	e_assert(ret, E_ERROR);
	return ret;
}

static e_int32 remove_pipe_handle(pipe_t *pip) {
	int flag = 1,ret,i;
	ret = mutex_lock(&pipes.mtx);
	e_assert(ret, E_ERROR);

	if (!pip->priv)
		return E_ERROR;

	pip->priv->ref--;
	if (!pip->priv->ref) {
		semaphore_destroy(&pip->priv->read_sem);
		semaphore_destroy(&pip->priv->write_sem);
		free(pip->priv->buf1);
		memset(pip->priv, 0, sizeof(struct pipe_handle_t));
		goto end;
	}

	flag = 0;
	for (i = 0; i < MAX_ENTITY_NUM; i++) { //remove
		if (pipes.handles[i].state) {
			flag = 1;
			break;
		}
	}
	end:
	if (!flag)
		mutex_destroy(&pipes.mtx);
	else
		mutex_unlock(&pipes.mtx);

	return E_OK;
}

e_int32 CPipe_Open(pipe_t *pip, char *name, int size)
{
	e_int32 ret;
	e_assert(pip, E_ERROR_INVALID_PARAMETER);

	memset(pip, 0, sizeof(pipe_t));

	ret = assign_pipe_handle(pip, name, size);
	e_assert(ret>0, ret);
	pip->state = TRUE;

	return E_OK;
}

e_int32 CPipe_Close(pipe_t *pip)
{
	e_int32 ret;
	e_assert(pip&&pip->state, E_ERROR_INVALID_PARAMETER);

	if (pip->priv)
	{
		ret = remove_pipe_handle(pip);
		e_assert(ret>0, ret);
	}
	pip->state = 0;
	return E_OK;
}

e_int32 CPipe_Timeouts(pipe_t *pip, int readTimeout_usec,
		int writeTimeout_usec)
{
	e_assert(pip&&pip->state, E_ERROR_INVALID_PARAMETER);
	pip->read_timeout_usec = readTimeout_usec;
	pip->write_timeout_usec = writeTimeout_usec;
	return E_OK;
}

e_int32 CPipe_Select(pipe_t *pip, e_int32 type, e_int32 timeout_usec)
{
	int ret;
	struct pipe_handle_t *pp;
	e_assert(pip&&pip->state, E_ERROR);

	pp = pip->priv;

	switch (type)
	{
	case E_READ:
		//DMSG(( STDOUT,"Com_Select FD=%d timeout=%dus",fd, timeout_usec));
		if (timeout_usec > 0)
				{
			ret = semaphore_timeoutwait(&pp->read_sem, timeout_usec);
		}
		else
		{
			ret = semaphore_wait(&pp->read_sem);
		}
		break;
	case E_WRITE:
		if (timeout_usec > 0)
				{
			ret = semaphore_timeoutwait(&pp->write_sem, timeout_usec);
		}
		else
		{
			ret = semaphore_wait(&pp->write_sem);
		}

		break;
	default:
		return E_ERROR_INVALID_PARAMETER;
	}

	e_assert(ret, E_ERROR);
	return E_OK;
}

#define swap_buf(_buf_,_buf1_,_buf2_) do{ if(_buf_ ==_buf1_) _buf_= _buf2_; else _buf_= _buf1_;}while(0)
static inline void inter_read_done(pipe_t *pip) {
	swap_buf(pip->priv->read_buf, pip->priv->buf1, pip->priv->buf2);
	semaphore_post(&pip->priv->write_sem);
}
static inline void inter_write_done(pipe_t *pip) {
	swap_buf(pip->priv->write_buf, pip->priv->buf1, pip->priv->buf2);
	semaphore_post(&pip->priv->read_sem);
}

e_int32 CPipe_Read(pipe_t *pip, e_uint8 *data, e_int32 size)
{
	e_assert(pip&&pip->state, E_ERROR);
	e_assert(size==pip->priv->size || size==0 || size==-1, E_ERROR_INVALID_PARAMETER);

	//简单实现全局读超时设置，待优化
	if (pip->read_timeout_usec > 0)
			{
		CPipe_Select(pip, E_READ, pip->read_timeout_usec);
	}

	memcpy(data, pip->priv->read_buf, size);
	inter_read_done(pip);
	return size;
}

e_int32 CPipe_Write(pipe_t *pip, e_uint8 *data, e_int32 size)
{
	e_assert(pip&&pip->state, E_ERROR);
	e_assert(size==pip->priv->size || size==0 || size==-1, E_ERROR_INVALID_PARAMETER);

	if (pip->write_timeout_usec > 0)
			{
		CPipe_Select(pip, E_WRITE, pip->write_timeout_usec);
	}

	memcpy(pip->priv->write_buf, data, size);
	inter_write_done(pip);
	return size;
}

