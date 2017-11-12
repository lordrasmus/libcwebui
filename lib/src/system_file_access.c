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

#include "webserver.h"
#include "miniz_tinfl.h"

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

	// TODO(lordrasmus) alle elemente richtig freigeben

	//freeFileCache();

	/* filepath erst NACH files freigeben weil der prefix direkt verlinkt wird */
	//free_local_file_system();

}
#include <zlib.h>

bool initFilesystem(void) {

	if (globals.init_called != 0xAB) {
		LOG(FILESYSTEM_LOG, ERROR_LEVEL, 0, "WebserverInit must be called first","");
		return false;
	}

	LOG(FILESYSTEM_LOG,NOTICE_LEVEL,0,"using zlib : %s",ZLIB_VERSION);

#ifdef WEBSERVER_USE_BINARY_FORMAT
	LOG( FILESYSTEM_LOG, NOTICE_LEVEL, 0, "using binary filesystem", "");
#endif


#ifdef WEBSERVER_USE_LOCAL_FILE_SYSTEM
	LOG( FILESYSTEM_LOG, NOTICE_LEVEL, 0, "using local filesystem", "");
#endif

#ifdef WEBSERVER_USE_WNFS
	LOG (FILESYSTEM_LOG,NOTICE_LEVEL,0, "using network filesystem","" );
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
	//printf("ram cached %s\n",file->FilePath);

	// local filesystem
	switch ( file->fs_type ){
		case FS_LOCAL_FILE_SYSTEM :
			local_file_system_read_content( file );
			break;
		case FS_BINARY :	// sind immer RAM cached
			break;
		case FS_WNFS:		// werden nie im RAM gecached
			printf("ERROR: ramCacheFile called for WNFS file\n");
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
			case FS_WNFS:		// werden nie im RAM gecached
				printf("ERROR: prepare_file_content RamCached WNFS file\n");
				break;
			case FS_LOCAL_FILE_SYSTEM :
				if ( 1 == local_file_system_check_file_modified( file ) ){
					printf("prepare_file_content:  file change %s File System -> FS_LOCAL_FILE_SYSTEM  \n", file->FilePath );
					#ifndef WEBSERVER_DISABLE_CACHE
					generateEtag ( file );
					#endif
				}
				break;
			case FS_BINARY :
				//printf("compressed : %d\n",file->Compressed);
				if ( file->Compressed == 2 ){
					//printf("warning: decompressing file : %s\n", file->FilePath );
					file->Data = WebserverMalloc( file->RealDataLenght );
					tinfl_decompress_mem_to_mem( (char*)file->Data, file->RealDataLenght, file->CompressedData, file->CompressedDataLenght, 0 );
					file->DataLenght = file->RealDataLenght;
				}
				return 1;
		}

		return 1;

	}else{

		if ( file->Data != 0 ) {
			printf("prepare_file_content:  File Data not 0 !!!! %s \n", file->FilePath );
			WebserverFree( ( void*) file->Data);
		}
	}

	// Datei ist nicht RamCached

	// local filesystem
	switch ( file->fs_type ){
		case FS_WNFS:		// WNFS wird vorher abgefragt
			break;
		case FS_LOCAL_FILE_SYSTEM :
			printf("prepare_file_content: prepare content %s  File System -> FS_LOCAL_FILE_SYSTEM \n", file->FilePath );
			local_file_system_read_content( file );
			break;

		case FS_BINARY :	// sind immer RAM cached
			break;
	}

	return 1;

}

void release_file_content(WebserverFileInfo* file) {

	if (file->RamCached == 1) {
		switch ( file->fs_type ){
			case FS_BINARY :	// wird für die template engine dekomprimiert
				if ( file->Compressed == 2 ){
					WebserverFree( (void*) file->Data );

					file->Data = file->CompressedData;
					file->DataLenght = file->CompressedDataLenght;
				}
				break;

			// local Filesystem muss nie dekomprimiert werden
			case FS_LOCAL_FILE_SYSTEM:
				break;
			
			case FS_WNFS:		// werden nie im RAM gecached
				printf("ERROR: release_file_content for RamCached WNFS file\n");
				break;
		}

		return;
	}

	WebserverFree( ( void*) file->Data);
	file->Data = 0;

}




WebserverFileInfo VISIBLE_ATTR *getFile( char *name)  {
	WebserverFileInfo *file = 0;

	if (name == 0){
		return 0;
	}

	while( name[0] == '/'){
		name++;
	}

	//printf("getFile : %s\n", name );

	if ( 1 == check_blocked_urls( name ) ){
		return 0;
	}


#ifdef WEBSERVER_USE_WNFS

	file = wnfs_get_file( name );
	if ( file != 0 ){
		return file;
	}

#endif

	file = getFileFromRBCache( name );
	if ( file != 0 ){
		#ifdef WEBSERVER_USE_WNFS
		wnfs_store_file( file );
		#endif
		return file;
	}


	file = getFileLocalFileSystem( name);
	if ( file == 0 ){
		printf("getFile : Error File %s not found\n", name );
		return 0;
	}



#ifndef WEBSERVER_DISABLE_CACHE
	generateEtag ( file );
#endif

	ramCacheFile(file);


	#ifdef WEBSERVER_USE_WNFS
	wnfs_store_file( file );
	#endif

	#ifndef DISABLE_OLD_TEMPLATE_SYSTEM
		// prüfen ob das File ein Template type ist ( depricated )
		file->TemplateFile = isTemplateFile(name);
	#endif
	return file;
}







