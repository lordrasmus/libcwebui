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

#include "intern/system_file_access.h"

static ws_variable* filepath;				/* suchpfade im localen filesystem */
static ws_variable* filepath_no_cache;		/* suchpfade im localen filesystem die nicht gecahed werden sollen */


void init_local_file_system( void ){

	filepath = newWSArray("dirs");
	filepath_no_cache = newWSArray("dirs_no_cache");

}

void free_local_file_system( void ){

	freeWSVariable(filepath);
	freeWSVariable(filepath_no_cache);

}

int local_file_system_check_file_modified( WebserverFileInfo *file ){

	int time_changed = 0;
	int new_size = 0;

	// TODO : Problem mit clients die die datei noch laden lösen

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

		PlatformOpenDataReadStream(file->FilePath);

		// zur sicherheit die länge mit geöffnetem filehandel nochmal lesen
		file->DataLenght = PlatformGetFileSize();

		WebserverFree( ( void* )file->Data );
		file->Data = (unsigned char*) WebserverMalloc( file->DataLenght );

		FILE_OFFSET ret = PlatformReadBytes( ( unsigned char*) file->Data, file->DataLenght);
		if ( ret != file->DataLenght ){
			//printf("Error: file size mismatch %jd != %jd\n",ret,file->DataLenght);
			printf("Error: file size mismatch1 %lu != %lu\n",ret,file->DataLenght);
		}

		PlatformCloseDataStream();
	}
	

	return 1;
}


int local_file_system_read_content( WebserverFileInfo *file ){

	if (PlatformOpenDataReadStream(file->FilePath)) {

		file->DataLenght = PlatformGetFileSize();
		file->Data = (unsigned char*) WebserverMalloc( file->DataLenght );

		FILE_OFFSET ret = PlatformReadBytes( ( unsigned char*) file->Data, file->DataLenght);
		if ( ret != file->DataLenght ){
			//printf("Error: file size mismatch %jd != %jd\n",ret,file->DataLenght);
			printf("Error: file size mismatch2 %lu != %lu\n",ret,file->DataLenght);
		}

		PlatformCloseDataStream();

		return 1;

	}else{
		return 0;
	}
}

void add_local_file_system_dir(const char* alias, const char* dir, const int use_cache){

	ws_variable *tmp;
	char buffer[1000];

	if ( use_cache == 1 ){

		tmp = getWSVariableArray(filepath, alias);
		if (tmp == 0) {
			tmp = addWSVariableArray(filepath, alias);
		}

	}else{

		tmp = getWSVariableArray(filepath_no_cache, alias);
		if (tmp == 0) {
			tmp = addWSVariableArray(filepath_no_cache, alias);
		}
	}

	strncpy(buffer, dir, 990);
	if (buffer[strlen(buffer)] != '/')
		strcat(buffer, "/");
	setWSVariableString(tmp, buffer);
}



static WebserverFileInfo* getFileInformation( const unsigned char *name) {
	unsigned char name_tmp[1000];
	ws_variable *tmp_var;
	WebserverFileInfo *file = 0;
	char found = 0;

	/*  um ein Zeichen weiterspringen wenn name mit / anfängt */
	while( name[0] == '/' )
		name++;

	/* Pfade für Datein die nicht gecached werden sollen */
	tmp_var = getWSVariableArrayFirst(filepath_no_cache);
	while (tmp_var != 0) {
		if (0 == strncmp(tmp_var->name, (char*) name, tmp_var->name_len)) {
			getWSVariableString(tmp_var, (char*) name_tmp, 1000);
			if (strlen( (char*) name) > tmp_var->name_len) {
				strcat( (char*)name_tmp, (char*)(name + tmp_var->name_len));
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
		/* Pfade für Datein die gecached werden dürfen */
		tmp_var = getWSVariableArrayFirst(filepath);

		if (tmp_var == 0) {
			printf("Keine Suchpfade fuer Datein registriert\n");
			return 0;
		}

		while (tmp_var != 0) {
			if (0 == strncmp(tmp_var->name, (char*) name, tmp_var->name_len)) {
				getWSVariableString(tmp_var, (char*)name_tmp, 1000);
				if (strlen( (char*) name) > tmp_var->name_len) {
					strcat( (char*) name_tmp, (char*)(name + tmp_var->name_len));
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

	copyFilePath(file, (unsigned char*) name_tmp);
	copyURL(file, name);

	/* tmp_var ist permanent in der liste der prefixe darum pointer direkt nehmen */
	file->FilePrefix = (unsigned char*) tmp_var->name;
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
			//printf("Error: file size mismatch %jd != %jd\n",ret, sizeof( template_v1_header ) -1 );
			printf("Error: file size mismatch3 %lu != %d\n",ret, sizeof( template_v1_header ) -1 );
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


WebserverFileInfo *getFileLocalFileSystem( const unsigned char *name) {
	WebserverFileInfo *file = 0;
	int a,b;

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




