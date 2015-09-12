/*

 libCWebUI
 Copyright (C) 2012  Ramin Seyed-Moussavi

 Projekt URL : http://code.google.com/p/libcwebui/

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

#ifdef __GNUC__
#include "webserver.h"
#endif

#ifdef DMALLOC
#include <dmalloc/dmalloc.h>
#endif

#include "intern/system_file_access.h"





void init_file_access( void ){

	initFileCache();

	init_file_access_utils();

	init_local_file_system();

}

void free_file_access( void ){

	#warning noch implementieren

	freeFileCache();

	/* filepath erst NACH files freigeben weil der prefix direkt verlinkt wird */
	free_local_file_system();

}


bool WebServerloadData(void) {

	if (globals.init_called != 0xAB) {
		LOG(FILESYSTEM_LOG, ERROR_LEVEL, 0, "WebserverInit must be called first","");
		return false;
	}

#ifdef WebserverUseNFS
	LOG (FILESYSTEM_LOG,NOTICE_LEVEL,0, "Webserver Load Data Use NFS","" );
#endif

#ifdef WEBSERVER_USE_LOCAL_FILE_SYSTEM
	LOG( FILESYSTEM_LOG, NOTICE_LEVEL, 0, "Webserver Load Data Use Local Filesystem", "");
#endif

	return true;

}



static char ramCacheFile(WebserverFileInfo *file) {

	if ( doNotRamCacheFile( file ) ){
		printf("not ram cached %s\n",file->FilePath);
		return 0;
	}

	if ( file->NoRamCache == 1 ){
		printf("not ram cached %s\n",file->FilePath);
		return 0;
	}

	if (file->FileType > WEBSERVER_MAX_FILEID_TO_RAM) {
		printf("not ram cached %s\n",file->FilePath);
		return 0;
	}

	file->RamCached = 1;
	printf("ram cached %s\n",file->FilePath);

	// local filesystem
	switch ( file->fs_type ){
		case FS_LOCAL_FILE_SYSTEM :
			local_file_system_read_content( file );
			break;
	}


	return 0;
}

int prepare_file_content(WebserverFileInfo* file) {

	if (file->RamCached == 1) {
		#ifdef WEBSERVER_DISABLE_FILE_UPDATE
			return 1;
		#endif

		switch ( file->fs_type ){
			case FS_LOCAL_FILE_SYSTEM :
				if ( 1 == local_file_system_check_file_modified( file ) ){
					printf("prepare_file_content:  file change %s File System -> FS_LOCAL_FILE_SYSTEM  \n", file->FilePath );
					#ifndef WEBSERVER_DISABLE_CACHE
					generateEtag ( file );
					#endif
				}
				break;
		}

		return 1;

	}else{

		if ( file->Data != 0 ) {
			printf("prepare_file_content:  File Data not 0 !!!! %s \n", file->FilePath );
			WebserverFree(file->Data);
		}
	}

	// Datei ist nicht RamCached

	// local filesystem
	switch ( file->fs_type ){
		case FS_LOCAL_FILE_SYSTEM :
			printf("prepare_file_content: prepare content %s  File System -> FS_LOCAL_FILE_SYSTEM \n", file->FilePath );
			local_file_system_read_content( file );
			break;
	}

	return 1;

}

void release_file_content(WebserverFileInfo* file) {

	if (file->RamCached == 1) {
		return;
	}

	WebserverFree(file->Data);
	file->Data = 0;

}




WebserverFileInfo *getFile(char *name) {
	WebserverFileInfo *file = 0;

	if (name == 0)
		return 0;

	while( name[0] == '/'){
		name++;
	}

	if ( 1 == check_blocked_urls( name ) ){
		return 0;
	}

	file = getFileFromRBCache( name );
	if ( file != 0 )
		return file;

	#ifdef WEBSERVER_USE_LOCAL_FILE_SYSTEM
		file = getFileLocalFileSystem(name);
		file->fs_type = FS_LOCAL_FILE_SYSTEM;
	#endif

	#ifdef WebserverUseNFS
		file = getFileNFS ( name );
	#endif

	if ( file == 0 ){
		return 0;
	}

#ifndef WEBSERVER_DISABLE_CACHE
	generateEtag ( file );
#endif

	ramCacheFile(file);

	// prüfen ob das File ein Template ist
	if (file != 0) {
		file->TemplateFile = isTemplateFile(name);
	}
	return file;
}







