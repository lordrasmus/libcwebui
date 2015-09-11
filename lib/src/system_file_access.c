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

#include "intern/system_file_access_utils.h"


void initFileCache(void);

extern ws_variable* filepath;
extern ws_variable* filepath_no_cache;


bool WebServerloadData(void) {

	if (globals.init_called != 0xAB) {
		LOG(FILESYSTEM_LOG, ERROR_LEVEL, 0, "WebserverInit must be called first","");
		return false;
	}

	init_file_access_utils();

#ifdef WebserverUseNFS
	LOG (FILESYSTEM_LOG,NOTICE_LEVEL,0, "Webserver Load Data Use NFS","" );
	return initLocalFileSystem();
#endif

#ifdef WEBSERVER_USE_LOCAL_FILE_SYSTEM
	LOG( FILESYSTEM_LOG, NOTICE_LEVEL, 0, "Webserver Load Data Use Local Filesystem", "");
#endif

	return true;

}



unsigned char getFileContents(WebserverFileInfo* info) {
#ifdef WebserverUseNFS
	// die datein sind immer RAM cached da sie komplette über ethernet geladen werden
#endif
#ifdef WEBSERVER_USE_BINARY_FORMAT
	if ( info->RamCached == 0 ) {
#ifdef _WEBSERVER_DEBUG_
#if __GNUC__ > 3
		LOG ( FILESYSTEM_LOG,NOTICE_LEVEL,"Flash Position %u ", ( unsigned int ) info->DataStreamPosition );
#else
		LOG ( FILESYSTEM_LOG,NOTICE_LEVEL,"Flash Position %u ",info->DataStreamPosition );
#endif
#endif
		PlatformOpenDataReadStream ( "data.bin" );
		PlatformSeekToPosition ( info->DataStreamPosition );
		PlatformReadBytes ( info->Data,info->DataLenght );
		PlatformCloseDataStream();
		return 1;
	}
	return 0;
#endif

#ifdef WEBSERVER_USE_LOCAL_FILE_SYSTEM
	if (info->RamCached == 0) {
		if (PlatformOpenDataReadStream(info->FilePath)) {
			PlatformReadBytes(info->Data, info->DataLenght);
			PlatformCloseDataStream();
			return 1;
		}
		return 0;
	}
	return 1;
#endif
}

#ifdef WEBSERVER_USE_LOCAL_FILE_SYSTEM

WebserverFileInfo VISIBLE_ATTR * getFileInformation(char *name) {
	char name_tmp[1000];
	ws_variable *tmp_var;
	WebserverFileInfo *file = 0;
	char found = 0;

	/*  um ein Zeichen weiterspringen wenn name mit / anfängt */
	if ( name[0] == '/' )
		name = name + 1;

	tmp_var = getWSVariableArrayFirst(filepath_no_cache);
	while (tmp_var != 0) {
		if (0 == strncmp(tmp_var->name, name, tmp_var->name_len)) {
			getWSVariableString(tmp_var, name_tmp, 1000);
			if (strlen(name) > tmp_var->name_len) {
				strcat(name_tmp, name + tmp_var->name_len);
				if (PlatformOpenDataReadStream(name_tmp)) {
					found = 2;
					break;
				}
			}
		}
		tmp_var = getWSVariableArrayNext(filepath_no_cache);
	}
	stopWSVariableArrayIterate(filepath_no_cache);

	if ( found == 0 ) {
		tmp_var = getWSVariableArrayFirst(filepath);

		if (tmp_var == 0) {
			printf("Keine Suchpfade fuer Datein registriert\n");
			return 0;
		}

		while (tmp_var != 0) {
			if (0 == strncmp(tmp_var->name, name, tmp_var->name_len)) {
				getWSVariableString(tmp_var, name_tmp, 1000);
				if (strlen(name) > tmp_var->name_len) {
					strcat(name_tmp, name + tmp_var->name_len);
					if (PlatformOpenDataReadStream(name_tmp)) {
						found = 1;
						break;
					}
				}
			}
			tmp_var = getWSVariableArrayNext(filepath);
		}
		stopWSVariableArrayIterate(filepath);
	}

	if (found == 0) {
		return 0;
	}

	file = (WebserverFileInfo*) WebserverMalloc ( sizeof ( WebserverFileInfo ) );
	memset(file, 0, sizeof(WebserverFileInfo));

	if (found == 2) {
		file->NoRamCache = 1;
	}

	copyFilePath(file, name_tmp);
	copyURL(file, name);
	/* tmp_var ist permanent in der liste der prefixe darum pointer direkt nehmen */
	file->FilePrefix = tmp_var->name;
	setFileType(file);
#ifdef _WEBSERVER_FILESYSTEM_CACHE_DEBUG_
	LOG (FILESYSTEM_LOG,NOTICE_LEVEL,0, "Add File Info Node","" );
#endif

	file->DataLenght = PlatformGetFileSize();
	PlatformCloseDataStream();
	addFileToCache(file);
	return file;
}




char ramCacheFile(WebserverFileInfo *file) {
	FILE_OFFSET to_read;
	int ret;

	if ( doNotRamCacheFile( file ) ){
		return 0;
	}


	if (file->FileType <= WEBSERVER_MAX_FILEID_TO_RAM) {
		if (PlatformOpenDataReadStream(file->FilePath)) {
			file->DataLenght = PlatformGetFileSize();
			file->RamCached = 1;
			if (file->Data != 0) WebserverFree(file->Data);
			file->Data = (unsigned char*) WebserverMalloc ( file->DataLenght );
			to_read = file->DataLenght;
			while (to_read > 0) {
				if ( to_read < INT_MAX ){
					ret = PlatformReadBytes(file->Data, to_read);
				}else{
					ret = PlatformReadBytes(file->Data, INT_MAX);
				}
				if (ret < 0) break;
				to_read -= (unsigned int )ret;
			}
			PlatformCloseDataStream();

		}
	}
	return 0;
}

static void updateLocalFileSize( WebserverFileInfo *file ){
	PlatformOpenDataReadStream(file->FilePath);
	file->DataLenght = PlatformGetFileSize();
	PlatformCloseDataStream();
}

static WebserverFileInfo *getFileLocalFileSystem(char *name) {
	WebserverFileInfo *file = 0;

#ifdef _WEBSERVER_FILESYSTEM_DEBUG_
	LOG (FILESYSTEM_LOG,NOTICE_LEVEL, 0,"getFileLocalFileSystem : %s",name );
#endif

	file = getFileFromRBCache(name);
	if (file == 0) {
		file = getFileInformation(name);
		if (file == 0) {
			return 0;
		}
		if ( doNotRamCacheFile( file ) ){
			return file;
		}

		PlatformGetFileTime ( file );
	} else {

		updateLocalFileSize( file );

		if ( doNotRamCacheFile( file ) ){
			return file;
		}

#ifdef WEBSERVER_DISABLE_FILE_UPDATE
		return file;
#endif

#ifndef WEBSERVER_DISABLE_CACHE
		if ( PlatformGetFileTime ( file ) == false) {
#ifdef _WEBSERVER_FILESYSTEM_CACHE_DEBUG_
			LOG (FILESYSTEM_LOG,NOTICE_LEVEL, 0,"File %s unveraendert",name );
#endif
			return file;
		}
#endif
	}

#ifdef _WEBSERVER_FILESYSTEM_CACHE_DEBUG_
	LOG ( FILESYSTEM_LOG,NOTICE_LEVEL,0,"File %s neu laden ",name);
#endif

	if ( doNotRamCacheFile( file ) ){
		return file;
	}

	ramCacheFile(file);

#ifndef WEBSERVER_DISABLE_CACHE
	generateEtag ( file );
#endif

	return file;
}

#endif







WebserverFileInfo *getFile(char *name) {
	WebserverFileInfo *file = 0;

	if (name == 0)
		return 0;

	if( name[0] == '/'){
		name++;
	}

	if ( 1 == check_blocked_urls( name ) ){
		return 0;
	}

#ifdef WEBSERVER_USE_LOCAL_FILE_SYSTEM
	file = getFileLocalFileSystem(name);
#endif

#ifdef WebserverUseNFS
	return getFileNFS ( name );
#endif

#ifdef WEBSERVER_USE_BINARY_FORMAT
	int i;
	for ( i=0;i<g_files.FileCount;i++ ) {
#ifdef _WEBSERVER_DEBUG_
		LOG (FILESYSTEM_LOG,NOTICE_LEVEL, ":%s:", ( char* ) g_files.files[i]->Name );
		LOG (FILESYSTEM_LOG,NOTICE_LEVEL, "(%d)",g_files.files[i]->NameLengt );
#endif
		/*if(strlen(g_files.files[i]->Name)<50)
		 LOG("\n%s(%d):%s\n",g_files.files[i]->Name,g_files.files[i]->NameLengt, name);
		 else{
		 LOG("\nDaten Kaputt %s\n",name);
		 }*/
		if ( 0==strncmp ( ( char* ) g_files.files[i]->Name, ( char* ) name,g_files.files[i]->NameLengt ) ) {
			//	LOG("\n%s(%d):%s\n",g_files.files[i]->Name,g_files.files[i]->NameLengt, name);
			//	LOG("DataLenght : %d %X\n",g_files.files[i]->DataLenght,g_files.files[i]);
			return g_files.files[i];
		}
	}
#endif

	if (file != 0) {
		file->TemplateFile = isTemplateFile(name);
	}
	return file;
}




