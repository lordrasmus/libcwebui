/*

 libCWebUI
 Copyright (C) 2007 - 2016  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

*/

#include "intern/system_file_access.h"

static ws_variable* file_dirs;

struct dir_info{
	char* alias;
	char* dir;
	
	int use_ram_cache;
	int auth_only;
};

void init_local_file_system( void ){

	file_dirs = newWSArray("dirs");

}

void free_local_file_system( void ){

	freeWSVariable(file_dirs);
#ifdef ENABLE_DEVEL_WARNINGS
	#warning free handler noch überarbeiten
#endif

}

int local_file_system_check_file_modified( WebserverFileInfo *file ){

	int time_changed = 0;
	int new_size = 0;

	// TODO(lordrasmus) : Problem mit clients die die datei noch laden lösen

	if ( 0 == PlatformGetFileInfo( file, &time_changed, &new_size )){
		printf("local_file_system_check_file_modified: Error file not found %s\n",file->FilePath);
		return 0;
	}

	if ( ( new_size == file->DataLenght ) && ( time_changed == 0 ) ){

		#ifdef _WEBSERVER_FILESYSTEM_CACHE_DEBUG_
			LOG (FILESYSTEM_LOG,NOTICE_LEVEL, 0,"File %s unveraendert",file->FilePath );
		#endif

		return 0;
	}


	if (file->RamCached == 0) {

		file->DataLenght = new_size;

		printf("local_file_system:  file change %s File System -> FS_LOCAL_FILE_SYSTEM \n", file->FilePath );

	}else{

		printf("local_file_system:  file change %s File System -> FS_LOCAL_FILE_SYSTEM ( RAM cached )\n", file->FilePath );

		// Datei ist im RAM cache
		PlatformOpenDataReadStream( file->FilePath );

		// zur sicherheit die länge mit geöffnetem filehandel nochmal lesen
		file->DataLenght = PlatformGetFileSize();

		WebserverFree( ( void* )file->Data );
		file->Data = (unsigned char*) WebserverMalloc( file->DataLenght );

		FILE_OFFSET ret = PlatformReadBytes( ( unsigned char*) file->Data, file->DataLenght);
		if ( ret != file->DataLenght ){
			printf("Error: file size mismatch1 %lu != %lu\n",ret,file->DataLenght);
		}

		PlatformCloseDataStream();
	}
	

	return 1;
}


int local_file_system_read_content( WebserverFileInfo *file ){

	if (PlatformOpenDataReadStream( file->FilePath )) {

		file->DataLenght = PlatformGetFileSize();
		file->Data = (unsigned char*) WebserverMalloc( file->DataLenght );

		FILE_OFFSET ret = PlatformReadBytes( ( unsigned char*) file->Data, file->DataLenght);
		if ( ret != file->DataLenght ){
			printf("Error: file size mismatch2 %lu != %lu\n",ret,file->DataLenght);
		}

		PlatformCloseDataStream();

		return 1;

	}else{
		return 0;
	}
}

void add_local_file_system_dir(const char* alias, const char* dir, const int use_cache, const int auth_only ){

	ws_variable *tmp;
	char buffer[1000];
	
	struct dir_info* info = WebserverMalloc( sizeof( struct dir_info ) ) ;

	if ( use_cache == 1 ){
		info->use_ram_cache = 1;
	}
	
	info->auth_only = auth_only;
	
	tmp = getWSVariableArray( file_dirs , alias);
	if (tmp != 0) {
		LOG( FILESYSTEM_LOG, ERROR_LEVEL, 0, "Error: dir alias %s exists",alias);
		exit(0);
	}
	
	LOG( FILESYSTEM_LOG, NOTICE_LEVEL, 0, "New dir alias /%s -> %s",alias,dir);
	
	tmp = addWSVariableArray( file_dirs , alias);
	
	strncpy(buffer, dir, 990);
	if (buffer[strlen(buffer)] != '/'){
		strcat(buffer, "/");
	}

	info->dir = WebserverMalloc( strlen( buffer ) + 1 );
	strcpy( info->dir, buffer);
	
	info->alias = WebserverMalloc( strlen( alias ) + 1 );
	strcpy( info->alias, alias);
	
	setWSVariableCustomData(tmp, 0, info);
	
}


static struct dir_info *search_file_dir ( const char* name, char* real_path, int real_path_length ){
	
	struct dir_info *dir_tmp = 0;
	ws_variable*tmp_var = getWSVariableArrayFirst( file_dirs );
	
	int found = 0;
	
	while (tmp_var != 0) {
		dir_tmp = tmp_var->val.value_p;
		
		real_path[0] = '\0';
		
		if (0 == strncmp(tmp_var->name, name, tmp_var->name_len)) {
			
			if (strlen( name ) > tmp_var->name_len ) {
				strncpy( real_path, dir_tmp->dir, real_path_length );
				
				int l = real_path_length - strlen( real_path );
			
				strncat( real_path, (char*)(name + tmp_var->name_len), l);
				if (PlatformOpenDataReadStream( real_path )) {
					found = 1;
					break;
				}
			}
		}
		tmp_var = getWSVariableArrayNext( file_dirs );
	}
	stopWSVariableArrayIterate( file_dirs );
	
	if ( found == 1 ){
		return dir_tmp;
	}
	
	return 0;
}


static WebserverFileInfo* getFileInformation( const char *name) {
	char name_tmp[1000];
	struct dir_info* dir = 0;
	WebserverFileInfo *file = 0;
	
	/*  um ein Zeichen weiterspringen wenn name mit / anfängt */
	while( name[0] == '/' ){
		name++;
	}
		
	
	dir = search_file_dir( name, name_tmp, 1000 );
	if (dir == 0) {
		return 0;
	}

	file = (WebserverFileInfo*) WebserverMalloc ( sizeof ( WebserverFileInfo ) );
	memset(file, 0, sizeof(WebserverFileInfo));

	if ( dir->use_ram_cache == 0 ){
		file->NoRamCache = 1;
	}
	
	file->auth_only = dir->auth_only;

	copyFilePath(file, name_tmp);
	copyURL(file, name);

	/* tmp_var ist permanent in der liste der prefixe darum pointer direkt nehmen */
	file->FilePrefix = dir->alias;
	setFileType(file);

#ifdef _WEBSERVER_FILESYSTEM_CACHE_DEBUG_
	LOG (FILESYSTEM_LOG,NOTICE_LEVEL,0, "Add File Info Node","" );
#endif

	file->DataLenght = PlatformGetFileSize();

	if ( file->DataLenght > (int)sizeof( template_v1_header ) ) {

		unsigned char template_header[sizeof( template_v1_header ) ];
		memset(template_header,0,sizeof( template_v1_header ) );

		FILE_OFFSET ret = PlatformReadBytes( template_header,  sizeof( template_v1_header ) -1 );
		if ( ret !=  sizeof( template_v1_header ) -1 ){
			printf("Error: file size mismatch3 %lu != %ld\n",ret, sizeof( template_v1_header ) -1 );
		}

		if ( 0 == memcmp( template_v1_header, template_header, sizeof( template_v1_header ) -1 ) ){
			//printf("getFileInformation: Engine Template V1 Header found : %s  \n", name_tmp);
			file->TemplateFile = 1;
		}
	}

	PlatformCloseDataStream();

	addFileToCache(file);
	return file;
}


WebserverFileInfo *getFileLocalFileSystem( const char *url_name) {
	WebserverFileInfo *file = 0;
	int a,b;
	
	char name[512];
	strncpy( name, url_name, 512 );
	name[511] = '\0';
	
	url_decode( name );

#ifdef _WEBSERVER_FILESYSTEM_DEBUG_
	LOG (FILESYSTEM_LOG,NOTICE_LEVEL, 0,"getFileLocalFileSystem : %s",name );
#endif

	file = getFileInformation(name);
	if (file == 0) {
		return 0;
	}

	file->fs_type = FS_LOCAL_FILE_SYSTEM;
	PlatformGetFileInfo ( file , &a, &b);

#ifdef _WEBSERVER_FILESYSTEM_CACHE_DEBUG_
	LOG ( FILESYSTEM_LOG,NOTICE_LEVEL,0,"File %s neu laden ",name);
#endif

	return file;
}




