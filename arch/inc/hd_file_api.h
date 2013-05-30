//----------------------------------------------------------------------------
//	hd_file_api.h
//  	uplusplus
// 	2013-5-7
//
//
//
//----------------------------------------------------------------------------
#ifndef _HD_FILE_API_H_
#define _HD_FILE_API_H_ 

#include <hd_plat_base.h>

//----------------------------------------------------------------------------
//	file mode
typedef enum{
	F_CREATE	=	0x0001,
	F_EXIST		=	0x0002,
	F_TEXT		=	0x0004,
	F_BINARY	=	0x0008,
	F_READ		=	0x00010,
	F_WRITE		=	0x0020,
	F_RDWR		=	0x0030,
	FSEEK_SET	=	0x1001,
	FSEEK_CUR	=	0x1002,
	FSEEK_END	=	0x1003
}FILE_PARAM;
//----------------------------------------------------------------------------
//	file_t definition
typedef struct file_t{
	e_uint32	fpid;					//	strean, file id, for nun-x86 system
	e_uint16	state;					//	file state(opend|closed)
	e_uint16	mode;					//	open mode(read|write|type)
	e_int8		filename[256];			//	file name
}file_t;

//----------------------------------------------------------------------------
//
#ifdef __cplusplus
extern "C"
{
#endif
//----------------------------------------------------------------------------
// file operation:
e_int32		DEV_EXPORT fi_open( const char *filename, e_int32 mode, file_t *fi ); 
void		DEV_EXPORT fi_close( file_t *fi );
e_int32		DEV_EXPORT fi_delete( const char *filename );
e_int32		DEV_EXPORT fi_rename( const char *newname, const char *oldname );

//----------------------------------------------------------------------------
// base file operation:	seek/read/write/flush/tell/seektobegin/seektoend
e_int32		DEV_EXPORT fi_seek( file_t *fi, e_int32 offset, e_int32 origin );
e_int32		DEV_EXPORT fi_read( void *buffer, e_int32 size, e_int32 count, file_t *fi );
e_int32		DEV_EXPORT fi_write( const void *buffer, e_int32 size, e_int32 count, file_t *fi );
e_int32		DEV_EXPORT fi_flush( file_t *fi );
e_int32		DEV_EXPORT fi_tell( file_t *fi );

#define 	fi_seektobegin( fi )	fi_seek( fi, 0, FSEEK_SET )
#define 	fi_seektoend( fi )		fi_seek( fi, 0, FSEEK_END )

//----------------------------------------------------------------------------
// base file operation:	state/mode
e_int32		DEV_EXPORT fi_opend( file_t *fi );
e_int32		DEV_EXPORT fi_mode( file_t *fi );

//----------------------------------------------------------------------------
//
#ifdef __cplusplus
}
#endif
//----------------------------------------------------------------------------
//

//----------------------------------------------------------------------------
//	directory struct definition
#define FTYPE_DIR	0
#define FTYPE_FILE	1

typedef struct{
	void			*priv;
	e_uint32	mode;
	
	char			name[256];
}dir_t;

typedef struct{
	e_uint32	type;
	char			name[128];
}finfo_t;

//----------------------------------------------------------------------------
//
#ifdef __cplusplus
extern "C"
{
#endif
//----------------------------------------------------------------------------
//	directory operation functions
int			DEV_EXPORT dir_open( const char *path, dir_t *dir );
void		DEV_EXPORT dir_close( dir_t *dir );
int			DEV_EXPORT dir_state( const dir_t *dir );

int			DEV_EXPORT dir_isexist( const char *path );
int			DEV_EXPORT dir_create( const char *path );
int			DEV_EXPORT dir_delete( const char *path );

int			DEV_EXPORT dir_findfirst( dir_t *dir, finfo_t *info );
int			DEV_EXPORT dir_findnext( dir_t *dir, finfo_t *info );

//----------------------------------------------------------------------------
//
#ifdef __cplusplus
}
#endif
//----------------------------------------------------------------------------
//
#endif	//	_HD_FILE_API_H_

//----------------------------------------------------------------------------
// EOF hd_file_api.h
