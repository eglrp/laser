//----------------------------------------------------------------------------
//	hd_file_api_android.c
//  author: Joy.you
// 	2013-05-08
//
//
//
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#include <arch/hd_file_api.h>
#include <arch/hd_mem_api.h>

#ifdef ANDROID_OS

//----------------------------------------------------------------------------
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <unistd.h>


#define  e_str2utf8( a, b, c ) do{strncpy(b,a,c);DMSG((STDOUT,"e_str2utf8 is not defined"));}while(0)
#define  e_utf82str( a, b, c ) do{strncpy(b,a,c);DMSG((STDOUT,"e_utf82str is not defined"));}while(0)

//----------------------------------------------------------------------------
void replace_splash(char *in_out)
{
	char *p = in_out;
	while(*p)
	{	
		if(*p == '\\') *p = '/';
		p++;
	}
	
	DMSG((STDOUT,"linux style file name: %s\r\n", in_out ));
}

char * cstr2utf8(const char * cstr)
{
	char *io = (char*)malloc(256);
	memset( io, '\0', 256 );
	e_str2utf8( cstr, io, 256 );
	replace_splash(io);
	return io;
}

char * utf82cstr(const char * utf8)
{
	char *io = (char*)malloc(256);
	memset( io, '\0', 256 );
	e_utf82str( utf8, io, 256 );
	replace_splash(io);
	return io;
}



//----------------------------------------------------------------------------
e_int32
fi_open( const char *filename, e_int32 mode, file_t *fi )
{
	FILE *pf = NULL;
	char ch_pos = 0, ch_mode[4]="";
	
	//	convert cstring 2 utf8 string
	char *_filename = cstr2utf8(filename);
	BZERO(fi, file_t );

	// get open mode---create/exist
	if( mode & F_CREATE ){
		ch_mode[ch_pos++] = 'w';
		if( mode & F_READ ){
			ch_mode[ch_pos++] = '+';
		}
	}
	else{
		ch_mode[ch_pos++] = 'r';
		if( mode & F_WRITE ){
			ch_mode[ch_pos++] = '+';
		}
	}

	//	get open type: text-binary
	if( mode & F_BINARY ){
		ch_mode[ch_pos++] = 'b';
	}
	else{
		ch_mode[ch_pos++] = 't';
	}

	//	open file to set parameters
	pf = fopen( _filename, ch_mode );
	free( _filename );
	
	if( pf == NULL ){
		DMSG((STDOUT,"failed to open file %s\r\n", filename ));
		perror( filename );
		return FALSE;
	}
	

	fi->fpid = (e_uint32)pf;
	sprintf( (char*)fi->filename, "%s", filename );
	fi->mode = (e_uint16)mode;
	fi->state = TRUE;

	DMSG((STDOUT,"success to open file %s\r\n", filename ));
	return TRUE;
}
				 
void
fi_close( file_t *fi )
{
	if( !fi->state )
		return;
		
	//	close file
	fi_flush( fi );
	fclose( (FILE*)fi->fpid );
	DMSG((STDOUT,"success to close file: %s\r\n",fi->filename));
	BZERO(fi, file_t );
}

e_int32		
fi_delete( const char *filename )
{
	//	convert cstring 2 utf8 string
	char *_filename = cstr2utf8(filename);
	int ret = remove( _filename );
	free( _filename );
	
	if( ret == 0 ){
		DMSG((STDOUT,"success to delete file: %s\r\n",filename));
		return 1;
	}else{
		DMSG((STDOUT,"failed to delete file: %s\r\n",filename));
		return 0;
	}
}

e_int32
fi_rename( const char *newname, const char *oldname )
{
	//	convert cstring 2 utf8 string
	char *_newname = cstr2utf8(newname);
	char *_oldname = cstr2utf8(oldname);
	int ret = rename( _oldname, _newname );
	free( _newname );
	free( _oldname );
	
	if( ret == 0 ){
		DMSG((STDOUT,"success to rename file: %s\r\n",oldname));
		return 1;
	}else{
		DMSG((STDOUT,"failed to rename file: %s\r\n",oldname));
		return 0;
	}
}

e_int32
fi_read( void *buffer, e_int32 size, e_int32 count, file_t *fi )
{
	return fread( buffer, size, count, (FILE*)fi->fpid );
}

e_int32
fi_write( const void *buffer, e_int32 size, e_int32 count, file_t *fi )
{
	return fwrite( buffer, size, count, (FILE*)fi->fpid );
}

e_int32
fi_flush( file_t *fi )
{
	return fflush( (FILE*)fi->fpid );
}

e_int32 
fi_seek( file_t *fi, e_int32 offset, e_int32 origin )
{
	e_int32 in_origin;
	switch( origin )
	{
	case FSEEK_SET:
		in_origin = SEEK_SET;break;
	case FSEEK_CUR:
		in_origin = SEEK_CUR;break;
	case FSEEK_END:
		in_origin = SEEK_END;break;
	default:
		return -1;
	}
	if( fseek( (FILE*)fi->fpid, offset, in_origin ) == 0 ){
		return ftell( (FILE*)fi->fpid );
	}
	return -1;
}

e_int32 
fi_tell( file_t *fi )
{
	return ftell( (FILE*)fi->fpid );
}

e_int32
fi_opend( file_t *fi )
{
	return fi->state;
}

e_int32
fi_mode( file_t *fi )
{
	return fi->mode;
}

//----------------------------------------------------------------------------
//	dir directory
int 
dir_open( const char *path, dir_t *dir )
{
	//	convert cstring 2 utf8 string
	char *_path = cstr2utf8(path);
	DIR *cdir = opendir( _path );
	free( _path );
	
	if( cdir == NULL ){
		return FALSE;
	}
	
	BZERO( dir, dir_t );
	dir->priv = (void*)cdir;
	dir->mode = TRUE;
	sprintf( dir->name, "%s", path );
	return TRUE;
}

void	   
dir_close( dir_t *dir )
{
	if( dir_state(dir) ){
		closedir( (DIR*)dir->priv );
	}

	BZERO( dir, dir_t );
}

int
dir_state( const dir_t *dir )
{
	return (dir->mode==TRUE)?TRUE:FALSE;
}

int	
dir_isexist( const char *path )
{
	//	convert cstring 2 utf8 string
	char *_path = cstr2utf8(path);
	int ret = access(_path, F_OK);
	free( _path );
	
	if( ret == 0 ){
		return TRUE;
	}else{
		return FALSE;
	}
}

int	
dir_create( const char *path )
{
	//	convert cstring 2 utf8 string
	char *_path = cstr2utf8(path);
	/* The behavior of mkdir is undefined for anything other than the “permission” bits */ 
	int ret = mkdir(_path, 0666);
	free( _path );
	
	if ( ret == 0 ){
		DMSG((STDOUT,"success to create dir: %s\r\n",path));
		return TRUE;
	}else{
		DMSG((STDOUT,"failed to create dir: %s\r\n",path));
		perror(path); 
		return FALSE;
	}
}

int	
dir_delete( const char *path )
{
	//	convert cstring 2 utf8 string
	char *_path = cstr2utf8(path);
	int ret = rmdir( _path );
	free( _path );
	
	if( ret == 0 ){
		DMSG((STDOUT,"success to delete dir: %s\r\n",path));
		return TRUE;
	}else{
		DMSG((STDOUT,"failed to delete dir: %s\r\n",path));
		return FALSE;
	}
}

int 
dir_findfirst( dir_t *dir, finfo_t *info )
{
	char *_name = NULL;
	struct dirent* ent = NULL;
	
	if( dir_state(dir) == FALSE )
		return FALSE;
	
	ent=readdir((DIR*)dir->priv );
	if( ent == NULL )
		return FALSE;
	
	_name = utf82cstr( ent->d_name );
	sprintf( info->name, "%s", _name );
	free( _name );
	
	if( ent->d_type == 4 ){
		info->type = FTYPE_DIR;
	}else{
		info->type = FTYPE_FILE;
	}
	
	return TRUE;
}

int 
dir_findnext( dir_t *dir, finfo_t *info )
{
	char *_name = NULL;
	struct dirent* ent = NULL;
	
	if( dir_state(dir) == FALSE )
		return FALSE;
	
	ent = readdir((DIR*)dir->priv);
	if( ent == NULL )
		return FALSE;
	
	_name = utf82cstr( ent->d_name );
	sprintf( info->name, "%s", _name );
	free( _name );
	
	if( ent->d_type == 4 ){
		info->type = FTYPE_DIR;
	}else{
		info->type = FTYPE_FILE;
	}
	
	return TRUE;
}

//----------------------------------------------------------------------------
#endif	//	ANDROID_OS
