#ifndef HD_PIPE_H
#define HD_PIPE_H

#include "hd_plat_base.h"

typedef struct pipe_handle_t pipe_handle;
#define PIPE_NAME_FRAME "PIPE_NAME_FRAME"
//----------------------------------------------------------------------------
//
typedef struct
{
	pipe_handle *priv; //	serial private handle
	e_uint8 name[MAX_PATH_LEN];
	e_int32 mode;
	e_uint32 read_timeout_usec;
	e_uint32 write_timeout_usec;
	e_uint32 state; //	serial state: open or not
} pipe_t;

e_int32 DEV_EXPORT Pipe_Open(pipe_t *pip, char *name,int size,int mode);
e_int32 DEV_EXPORT Pipe_Close(pipe_t *pip);
e_int32 DEV_EXPORT Pipe_Timeouts(pipe_t *pip,int readTimeout_usec, int writeTimeout_usec);
e_int32 DEV_EXPORT Pipe_Select(pipe_t *pip,e_int32 type,e_int32 timeout_usec);
e_int32 DEV_EXPORT Pipe_Read(pipe_t *pip, e_uint8 *data, e_int32 size);
e_int32 DEV_EXPORT Pipe_Write(pipe_t *pip, e_uint8 *data, e_int32 size);

#endif /* !HD_PIPE_H */
