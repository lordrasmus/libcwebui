/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/

#ifdef __CDT_PARSER__
	#define __BASE_FILE__ base
#endif

#include "intern/system_file_access.h"

struct dir_info{
	char* alias;
	size_t alias_len;
	char* dir;

	int use_ram_cache;
	int auth_only;

	struct dir_info* next;
};

/* The registered directories, longest alias first, aliases of the same
 * length in alphabetical order - kept that way when one is added. The
 * first alias in the list that a URL starts with is therefore the longest
 * one: download/licence/ is tried before download/ and that before the
 * root "", whatever order they were registered in. If the file is not in
 * the first one, the next shorter matching alias is tried, as before. */
static struct dir_info* dir_list;

void init_local_file_system( void ){

	dir_list = 0;

}

void free_local_file_system( void ){

	while ( dir_list != 0 ){
		struct dir_info* info = dir_list;
		dir_list = info->next;
		WebserverFree( info->alias );
		WebserverFree( info->dir );
		WebserverFree( info );
	}

}

/* Whether a goes before b in dir_list */
static int dir_alias_before( const struct dir_info* a, const struct dir_info* b ){
	if ( a->alias_len != b->alias_len ){
		return a->alias_len > b->alias_len;
	}
	return strcmp( a->alias, b->alias ) < 0;
}

int local_file_system_check_file_modified( WebserverFileInfo *file ){

	int time_changed = 0;
	FILE_OFFSET new_size = 0;

	/* Responses still sending the cached content read Data and DataLenght
	 * anew on every write event: reloading now would let them go on with
	 * the new content from their old position, under the Content-Length of
	 * the old one. Not even checked meanwhile - the check itself already
	 * takes over the new modification time and Last-Modified text, and the
	 * change would be forgotten. The first request after the last of those
	 * responses has finished finds the change and loads it. */
	if ( ( file->RamCached == 1 ) && ( file->transfers > 0 ) ){
		return 0;
	}

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

		//printf("local_file_system:  file change %s File System -> FS_LOCAL_FILE_SYSTEM ( RAM cached )\n", file->FilePath );

		// Datei ist im RAM cache
		/* Gone between the check above and here: keep what is cached, as
		 * for a file not found at all - reading on would use a stream that
		 * is not open */
		if ( !PlatformOpenDataReadStream( file->FilePath ) ){
			printf("local_file_system_check_file_modified: Error cannot open %s\n",file->FilePath);
			return 0;
		}

		// zur sicherheit die länge mit geöffnetem filehandel nochmal lesen
		file->DataLenght = PlatformGetFileSize();

		WebserverFree( ( void* )file->Data );
		file->Data = (unsigned char*) WebserverMalloc( file->DataLenght );

		FILE_OFFSET ret = PlatformReadBytes( ( unsigned char*) file->Data, file->DataLenght);
		if ( ret != file->DataLenght ){
			printf("Error: file size mismatch1 %"FILE_OFF_PRINT_INT" != %"FILE_OFF_PRINT_INT"\n",FILE_OFF_CAST(ret),FILE_OFF_CAST(file->DataLenght));
			/* Only what was read: the rest of the allocation is not file
			 * content and must not be served as such */
			file->DataLenght = ret;
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
			printf("Error: file size mismatch2 %"FILE_OFF_PRINT_INT" != %"FILE_OFF_PRINT_INT"\n",FILE_OFF_CAST(ret),FILE_OFF_CAST(file->DataLenght));
		}

		PlatformCloseDataStream();

		return 1;

	}else{
		return 0;
	}
}

void add_local_file_system_dir(const char* alias, const char* dir, const int use_cache, const int auth_only ){

	struct dir_info** pos;
	size_t len;
	char* alias_slash;

	struct dir_info* info = WebserverMalloc( sizeof( struct dir_info ) ) ;
	/* WebserverMalloc() does not clear: use_ram_cache was only ever set to
	 * 1, so a directory registered without the RAM cache got whatever the
	 * block held before - and was cached after all if that was not 0 */
	memset( info, 0, sizeof( struct dir_info ) );

	info->use_ram_cache = ( use_cache == 1 ) ? 1 : 0;

	info->auth_only = auth_only;

	/* The alias always ends with '/'. search_file_dir() compares it as a
	 * prefix of the URL; without the '/' "down" also matched "download/x"
	 * and looked for load/x in down's directory - only the order of
	 * registration kept that away. With it the alias only matches a URL
	 * going on below it, in any order. An alias always names a directory:
	 * search_file_dir() needs something after it.
	 * The empty alias stays empty: it is the root (the web interface
	 * registers its pages that way) and is meant to match every URL. */
	len = strlen( alias );
	alias_slash = WebserverMalloc( len + 2 );
	memcpy( alias_slash, alias, len );
	if ( ( len > 0 ) && ( alias[len - 1] != '/' ) ){
		alias_slash[len++] = '/';
	}
	alias_slash[len] = '\0';

	for ( pos = &dir_list; *pos != 0; pos = &(*pos)->next ){
		if ( 0 == strcmp( (*pos)->alias, alias_slash ) ){
			LOG( FILESYSTEM_LOG, ERROR_LEVEL, 0, "Error: dir alias %s exists",alias_slash);
			exit(0);
		}
	}

	LOG( FILESYSTEM_LOG, NOTICE_LEVEL, 0, "New dir alias /%s -> %s",alias_slash,dir);

	/* The directory with exactly one '/' at the end. The old copy through
	 * a 1000 byte buffer left it unterminated for 990 characters or more,
	 * and tested the terminator instead of the last character, so every
	 * directory got a second '/'. */
	len = strlen( dir );
	info->dir = WebserverMalloc( len + 2 );
	memcpy( info->dir, dir, len );
	if ( ( len == 0 ) || ( dir[len - 1] != '/' ) ){
		info->dir[len++] = '/';
	}
	info->dir[len] = '\0';
	
	info->alias = alias_slash;
	info->alias_len = strlen( alias_slash );

	/* Into its place in the order of dir_list */
	for ( pos = &dir_list; ( *pos != 0 ) && dir_alias_before( *pos, info ); pos = &(*pos)->next ){
	}
	info->next = *pos;
	*pos = info;

}


static struct dir_info *search_file_dir ( const char* name, char* real_path, int real_path_length ){
	
	struct dir_info *dir_tmp;

	/* Longest alias first, see dir_list */
	for ( dir_tmp = dir_list; dir_tmp != 0; dir_tmp = dir_tmp->next ) {

		real_path[0] = '\0';

		if (0 == strncmp(dir_tmp->alias, name, dir_tmp->alias_len)) {

			if (strlen( name ) > dir_tmp->alias_len ) {
				/* Directory and the rest of the URL. strncat() with the
				 * room left wrote its terminator one byte past the buffer,
				 * and a path cut to fit named another file than the one
				 * asked for: one that does not fit is not in this
				 * directory. */
				/* dir ends in exactly one '/', the rest of the URL starts
				 * with one unless the alias was registered with it */
				const char* rest = name + dir_tmp->alias_len;
				while ( rest[0] == '/' ){
					rest++;
				}
				int n = snprintf( real_path, (size_t)real_path_length, "%s%s",
				                  dir_tmp->dir, rest );
				if ( ( n < 0 ) || ( n >= real_path_length ) ){
					real_path[0] = '\0';
					continue;
				}
				if (PlatformOpenDataReadStream( real_path )) {
					return dir_tmp;
				}
			}
		}
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

	/* dir stays in dir_list for good, so its alias can be referenced */
	file->FilePrefix = dir->alias;
	setFileType(file);

#ifdef _WEBSERVER_FILESYSTEM_CACHE_DEBUG_
	LOG (FILESYSTEM_LOG,NOTICE_LEVEL,0, "Add File Info Node" );
#endif

	file->DataLenght = PlatformGetFileSize();

	if ( file->DataLenght > (int)sizeof( template_v1_header ) ) {

		unsigned char template_header[sizeof( template_v1_header ) ];
		memset(template_header,0,sizeof( template_v1_header ) );

		FILE_OFFSET ret = PlatformReadBytes( template_header,  sizeof( template_v1_header ) -1 );
		if ( ret !=  sizeof( template_v1_header ) -1 ){
			printf("Error: file size mismatch3 %"FILE_OFF_PRINT_INT" != %zu\n",FILE_OFF_CAST(ret), sizeof( template_v1_header ) -1 );
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
	int a;
	FILE_OFFSET b;

	char name[512];
	strncpy( name, url_name, 512 );
	name[511] = '\0';

	/* URL is already decoded in header_parser.c - do NOT decode again!
	 * Double decoding allows directory traversal via %252e%252e */

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




