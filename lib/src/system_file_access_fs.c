

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

	}else{

		// Datei ist im RAM cache

		PlatformOpenDataReadStream(file->FilePath);

		// zur sicherheit die länge mit geöffnetem filehandel nochmal lesen
		file->DataLenght = PlatformGetFileSize();

		WebserverFree( ( void* )file->Data );
		file->Data = (unsigned char*) WebserverMalloc( file->DataLenght );
		PlatformReadBytes( ( unsigned char*) file->Data, file->DataLenght);

		PlatformCloseDataStream();
	}

	return 1;
}


int local_file_system_read_content( WebserverFileInfo *file ){

	if (PlatformOpenDataReadStream(file->FilePath)) {

		file->DataLenght = PlatformGetFileSize();
		file->Data = (unsigned char*) WebserverMalloc( file->DataLenght );
		PlatformReadBytes( ( unsigned char*) file->Data, file->DataLenght);
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

	strcpy(buffer, dir);
	if (dir[strlen(dir)] != '/') strcat(buffer, "/");
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




