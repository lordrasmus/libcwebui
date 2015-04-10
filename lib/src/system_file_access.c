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

void initFileCache(void);

extern ws_variable* filepath;
extern ws_variable* filepath_no_cache;


static ws_variable* no_ram_cache_files;

bool initLocalFileSystem(void) {
	initFileCache();
	no_ram_cache_files = newWSVariable("no_ram_cache_files");
	setWSVariableArray(no_ram_cache_files);
	return true;
}

void generateEtag(WebserverFileInfo* wfi) {
#ifdef WEBSERVER_USE_SSL
	unsigned char buf[SSL_SHA_DIG_LEN];
	if (wfi->etag != 0) WebserverFree(wfi->etag);
	wfi->etag = (char *) WebserverMalloc ( SSL_SHA_DIG_LEN * 2 + 1 );
	memset(wfi->etag,0,SSL_SHA_DIG_LEN*2+1);

	if (wfi->RamCached == 1) {
		WebserverSHA1(wfi->Data, wfi->DataLenght, buf);
	} else {
		struct sha_context* sha_context;
		unsigned int to_read;
		unsigned long diff;
		FILE_OFFSET pos;
		unsigned char *data = (unsigned char*) WebserverMalloc ( WRITE_DATA_SIZE );

		pos = 0;
		sha_context = WebserverSHA1Init();

		if (!PlatformOpenDataReadStream(wfi->FilePath)) return;
		PlatformSeekToPosition(0);

		while (1) {
			diff = wfi->DataLenght - pos;

			if (diff > WRITE_DATA_SIZE) {
				to_read = WRITE_DATA_SIZE;
			} else {
				to_read = diff;
			}

			PlatformReadBytes(data, to_read);
			WebserverSHA1Update(sha_context, data, to_read);
			pos += to_read;

			if (pos == wfi->DataLenght) break;

		}
		WebserverSHA1Final(sha_context, buf);
		PlatformCloseDataStream();
		WebserverFree(data);
	}

	convertBinToHexString(buf, SSL_SHA_DIG_LEN, wfi->etag, SSL_SHA_DIG_LEN * 2 + 1);
	wfi->etagLength = strlen(wfi->etag);

#else

	if (wfi->etag == 0) {
		wfi->etag = (char *) WebserverMalloc( 100 );
	}

	wfi->etagLength = sprintf((char*) wfi->etag, "Test %s", wfi->Url);
	wfi->etagLength = 100;

#ifdef _WIN32
#pragma message ( "etag generierung ohne ssl richtig implementieren" )
#elif defined (__GNUC__)
#warning "etag generierung ohne ssl richtig implementieren"
#else
#error "etag generierung ohne ssl richtig implementieren"
#endif

#endif

}

void setFileType(WebserverFileInfo* file) {
	unsigned int i;
	int len;
	static char ext[11];

	file->FileType = FILE_TYPE_PLAIN;

	for (len = strlen((char*) file->Url) - 1; len >= 0; len--){
		if (file->Url[len] == '.'){
			break;
		}

		if (file->Url[len] == '/'){
			break;
		}
	}

	if (len == -1) {
		return;
	}

	if ( ( strlen((char*) file->Url) - len ) <= 10 ){
		for (i = 0; i < strlen((char*) file->Url) - len; i++)
			ext[i] = tolower(file->Url[i + len]);
		ext[i] = '\0';
	}else{
		ext[0] = '\0';
	}

	if (0 == strncmp((char*) ext, ".html", 10))
		file->FileType = FILE_TYPE_HTML;
	else if (0 == strncmp((char*) ext, ".inc", 10))
		file->FileType = FILE_TYPE_HTML_INC;
	else if (0 == strncmp((char*) ext, ".css", 10))
		file->FileType = FILE_TYPE_CSS;
	else if (0 == strncmp((char*) ext, ".js", 10))
		file->FileType = FILE_TYPE_JS;
	else if (0 == strncmp((char*) ext, ".ico", 10))
		file->FileType = FILE_TYPE_ICO;
	else if (0 == strncmp((char*) ext, ".gif", 10))
		file->FileType = FILE_TYPE_GIF;
	else if (0 == strncmp((char*) ext, ".jpg", 10))
		file->FileType = FILE_TYPE_JPG;
	else if (0 == strncmp((char*) ext, ".png", 10))
		file->FileType = FILE_TYPE_PNG;
	else if (0 == strncmp((char*) ext, ".xml", 10))
		file->FileType = FILE_TYPE_XML;
	else if (0 == strncmp((char*) ext, ".xsl", 10))
		file->FileType = FILE_TYPE_XSL;
	else if (0 == strncmp((char*) ext, ".manifest", 10))
		file->FileType = FILE_TYPE_MANIFEST;
	else if (0 == strncmp((char*) ext, ".svg", 10))
		file->FileType = FILE_TYPE_SVG;
	else if (0 == strncmp((char*) ext, ".bmp", 10))
		file->FileType = FILE_TYPE_BMP;
	else if (0 == strncmp((char*) ext, ".pdf", 10))
		file->FileType = FILE_TYPE_PDF;
	else if (0 == strncmp((char*) ext, ".json", 10))
		file->FileType = FILE_TYPE_JSON;
	else if (0 == strncmp((char*) ext, ".woff", 10))
		file->FileType = FILE_TYPE_WOFF;
	else if (0 == strncmp((char*) ext, ".eot", 10))
		file->FileType = FILE_TYPE_EOT;
	else if (0 == strncmp((char*) ext, ".ttf", 10))
		file->FileType = FILE_TYPE_TTF;
	else
		LOG(FILESYSTEM_LOG, NOTICE_LEVEL, 0, "%s is FILE_TYPE_PLAIN",
				file->Url);
}

bool WebServerloadData(void) {

	if (globals.init_called != 0xAB) {
		LOG(FILESYSTEM_LOG, ERROR_LEVEL, 0, "WebserverInit must be first call",
				"");
		return false;
	}

#ifdef WebserverUseNFS
	LOG (FILESYSTEM_LOG,NOTICE_LEVEL,0, "Webserver Load Data Use NFS","" );
	return initLocalFileSystem();
#endif

#ifdef WEBSERVER_USE_LOCAL_FILE_SYSTEM
	LOG( FILESYSTEM_LOG, NOTICE_LEVEL, 0, "Webserver Load Data Use Local Filesystem", "");
	return initLocalFileSystem();
#endif

#ifdef WEBSERVER_USE_BINARY_FORMAT
	int i; //,i2,i3;
//int sig[4];
//unsigned long flashposition=0;
	WebserverFileInfo *info;
	unsigned int DataLenght;
	unsigned long to_read,read_pos;

	WDT_RESET

	if ( PlatformOpenDataReadStream ( "data.bin" ) ==false ) {
		LOG (FILESYSTEM_LOG,ERROR_LEVEL, "Webserver Load Data failed\r\n" );
		return false;
	}
	//WebserverPrintPosition();

	WDT_RESET

	LOG ( FILESYSTEM_LOG,NOTICE_LEVEL,"Webserver Load Data\r\n" );

	DataLenght = readInt();
	g_lastmodified=WebserverMalloc ( DataLenght + 1 );
	PlatformReadBytes ( g_lastmodified,DataLenght );
	g_lastmodified[DataLenght]='\0';

#ifdef _WEBSERVER_DEBUG_
	LOG ( FILESYSTEM_LOG,NOTICE_LEVEL,"Last Modified Lenght:  %u\n",DataLenght );
	LOG ( FILESYSTEM_LOG,NOTICE_LEVEL,"Last Modified :  %s\n",g_lastmodified );
#endif

	g_files.FileCount = readInt();
	g_files.files= ( WebserverFileInfo** ) WebserverMalloc ( sizeof ( WebserverFileInfo* ) * g_files.FileCount );

#ifdef _WEBSERVER_DEBUG_
	LOG (FILESYSTEM_LOG,NOTICE_LEVEL, "Files : %d\n",g_files.FileCount );
#endif

	for ( i=0;i<g_files.FileCount;i++ ) {
		WDT_RESET

		info = ( WebserverFileInfo* ) WebserverMalloc ( sizeof ( WebserverFileInfo ) );
		memset ( info,0,sizeof ( WebserverFileInfo ) );
		if ( info==0 ) {
			LOG ( FILESYSTEM_LOG,ERROR_LEVEL,"WebServerloadData Malloc FileInfo Struct : Kein Speicher verfuegbar\n" );
			return false;
		}

		info->NameLengt = readInt();
		info->Name = ( unsigned char* ) WebserverMalloc ( info->NameLengt + 1 );
		if ( info->Name==0 ) {
			LOG ( FILESYSTEM_LOG,ERROR_LEVEL,"WebServerloadData Malloc Name : Kein Speicher verfuegbar (%d)\n",info->NameLengt );
			return false;
		}
		PlatformReadBytes ( info->Name,info->NameLengt );
		info->Name[info->NameLengt]='\0';

		PlatformReadBytes ( & ( info->FileType ),1 );
		PlatformReadBytes ( & ( info->Id ),1 );

		setFileType ( info );
		info->DataLenght = readInt();
//if ((info->FileType == FILE_TYPE_HTML)||(info->FileType == FILE_TYPE_HTML_INC)) 			// Daten in den Speicher laden ( HTML , Text)
		if ( info->FileType <= WEBSERVER_MAX_FILEID_TO_RAM ) {
			info->Data = ( unsigned char* ) WebserverMalloc ( sizeof ( unsigned char ) *info->DataLenght );
			if ( info->Data==0 ) {
				LOG (FILESYSTEM_LOG,ERROR_LEVEL, "WebServerloadData : Kein Speicher verfuegbar\n" );
				return false;
			}
			to_read = info->DataLenght;
			read_pos = 0;
			while ( to_read >= 1000 ) {
				PlatformReadBytes ( &info->Data[read_pos],1000 );
				to_read -= 1000;
				read_pos += 1000;
			}
			PlatformReadBytes ( &info->Data[read_pos],to_read );

			info->RamCached = 1;
		} else { // Position der Daten im Datenstream merken
			info->DataStreamPosition=PlatformGetDataStreamPosition();
			PlatformSeek ( info->DataLenght );
			//WebserverSeekToPosition(info->DataLenght);
		}

#ifdef _WEBSERVER_DEBUG_
		LOG (FILESYSTEM_LOG,NOTICE_LEVEL, "Name : %s (%d) ID : %d\n",info->Name,info->NameLengt,info->Id );
		LOG ( FILESYSTEM_LOG,NOTICE_LEVEL,"Datalenght %lu\n\n",info->DataLenght );
//LOG("File : %s ",info->Name);
//WebserverPrintPosition();
#endif

//      WebserverSeekToPosition(
//		WebserverReadBytes(debug_buffer,info->DataLenght);

		g_files.files[i]=info;

	}

	PlatformCloseDataStream();

	return true;
#endif
}

#ifdef WebserverUseNFS
//char nfsbuffer[50000];
static socket_info NFSSocket;

bool connectNFSServer ( void ) {
	static char ip[4];

	ip[0]=WEBSERVER_NFS_IP1;
	ip[1]=WEBSERVER_NFS_IP2;
	ip[2]=WEBSERVER_NFS_IP3;
	ip[3]=WEBSERVER_NFS_IP4;

	//LOG("Webserver Connect NFS\n");
	NFSSocket.socket = PlatformCreateSocket();
	return PlatformConnectSocket ( NFSSocket.socket,ip,5555 );

	//status = connect(NFSSocket.socket, (struct sockaddr *)&dstAddr, sizeof(dstAddr));
}
#endif

void copyFilePath(WebserverFileInfo* file, char* name) {
	file->FilePathLengt = strlen((char*) name);
	if (file->FilePath != 0) {
		WebserverFree(file->FilePath);
	}
	file->FilePath = (char*) WebserverMalloc(file->FilePathLengt + 1 );
	strncpy((char*) file->FilePath, (char*) name, file->FilePathLengt);
	file->FilePath[file->FilePathLengt] = '\0';
}

void copyURL(WebserverFileInfo* file, char* name) {
	file->UrlLengt = strlen((char*) name);
	if (file->Url != 0) {
		WebserverFree(file->Url);
	}
	file->Url = (char*) WebserverMalloc( file->UrlLengt + 1 );
	strncpy((char*) file->Url, (char*) name, file->UrlLengt);
	file->Url[file->UrlLengt] = '\0';
}

WebserverFileInfo *create_empty_file(int pSize) {
	WebserverFileInfo *result = (WebserverFileInfo *) WebserverMalloc( sizeof(WebserverFileInfo) );
	if (!result)
		return 0;
	result->FileType = FILE_TYPE_HTML;
	result->Data = (unsigned char *) WebserverMalloc( pSize );
	if (!result->Data)
		return 0;
	result->DataLenght = 0;
	return result;
}

void free_file(WebserverFileInfo *file) {
	if (file->Data)
		WebserverFree(file->Data);
	WebserverFree(file);
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

WebserverFileInfo *getFileInformation(char *name) {
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




static int doNotRamCacheFile( WebserverFileInfo *file ){
	ws_variable *tmp;
	if ( file->NoRamCache == 1 ){
		return 1;
	}

	tmp = getWSVariableArrayFirst(no_ram_cache_files);
	while (tmp != 0) {
		if( 0 == strcmp( tmp->name , file->Url ) ){
			stopWSVariableArrayIterate(no_ram_cache_files);
			file->NoRamCache = 1;
			return 1;
		}
		tmp = getWSVariableArrayNext(no_ram_cache_files);
	}
	stopWSVariableArrayIterate(no_ram_cache_files);

	return 0;
}

void WebserverAddNoRamCacheFile( char* url ){
	ws_variable *tmp;

	if( url[0] == '/'){
		url++;
	}
	tmp = getWSVariableArray(no_ram_cache_files, url);

	if (tmp == 0) {
		tmp = addWSVariableArray(no_ram_cache_files, url);
	}
	setWSVariableInt(tmp, 1);
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

WebserverFileInfo *getFileLocalFileSystem(char *name) {
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

#ifdef WebserverUseNFS
WebserverFileInfo *getFileNFS ( char *name ) {
	unsigned long recv,le,ret;
	static char buffer[300];
	char cle;

#ifdef _WEBSERVER_FILESYSTEM_DEBUG_
	LOG (FILESYSTEM_LOG,NOTICE_LEVEL, "Get File Use NFS : %s\r\n",name );
#endif

	if ( false == connectNFSServer() ) {
		LOG (FILESYSTEM_LOG,ERROR_LEVEL,"NFS Server not avaible" );
		return 0;
	}

	for ( i=0;i<WEBSERVER_RAM_FILE_SPACE;i++ ) {
		if ( 0==strcmp ( ( char* ) g_files.files[i]->Name, ( char* ) name ) ) {
			break;
		}
	}
	if ( i==WEBSERVER_RAM_FILE_SPACE ) {
		for ( i=0;i<WEBSERVER_RAM_FILE_SPACE;i++ ) {
			if ( g_files.files[i]->Used==0 )
			break;
		}
	}

	g_files.files[i]->Id=0;
	g_files.files[i]->Used=1;
	g_files.files[i]->RamCached=1;

	copyFileName ( g_files.files[i],name );
	setFileType ( g_files.files[i] );

	cle= sprintf ( buffer,"getFile:%s\0",name );
	WebserverPlatformSend ( &NFSSocket,&cle,1,0 );
	WebserverPlatformSend ( &NFSSocket,buffer,cle,0 );
	WebserverPlatformRecv ( &NFSSocket,buffer,1,0 );
	//g_files.files[i]->FileType = buffer[0];
	WebserverPlatformRecv ( &NFSSocket,buffer,4,0 );
	le= buffer[0]<<24;
	le+= buffer[1]<<16;
	le+= buffer[2]<<8;
	le+= ( unsigned char ) buffer[3];

	cle=i;
	g_files.files[i]->DataLenght = le;
	if ( g_files.files[i]->Data != 0 )
	WebserverFree ( g_files.files[i]->Data );
	g_files.files[i]->Data = ( unsigned char* ) WebserverMalloc ( sizeof ( unsigned char ) * ( le ) );
	recv=0;
	do {
		ret=WebserverPlatformRecv ( &NFSSocket,&g_files.files[i]->Data[recv],le-recv,0 );
		recv+=ret;
	}while ( recv<le );

	//LOG("Data : %s\r\n",g_files.files[i]->Data);

	WebserverCloseSocket ( &NFSSocket );
	if ( le == 0 )
	return 0;

	return g_files.files[i];
}

#endif

WebserverFileInfo *getFile(char *name) {
	WebserverFileInfo *file = 0;

	if (name == 0)
		return 0;

	if( name[0] == '/'){
		name++;
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

char WebServerReadDataStart(void) {
	unsigned int data[5];
	data[0] = readInt(); /* signatur */
	data[1] = readInt(); 
	data[2] = readInt(); 
	data[3] = readInt(); 

	data[4] = readInt(); /* Laenge des Datensatzes */

	if ((data[0] != 1) || (data[1] != 255) || (data[2] != 1)
			|| (data[3] != 255)) {
		LOG(FILESYSTEM_LOG, ERROR_LEVEL, 0,
				"Webserver Datensignatur nicht gefunden (0x%X,0x%X,0x%X,0x%X) !!!",
				data[0], data[1], data[2], data[3]);
		return false;
	}

	return true;
}

#ifdef WebserverUseNFS
extern socket_info NFSSocket;
#endif

#ifdef jio

void WebserverSaveDataChunk ( unsigned char *data,unsigned int lenght ) {
	PlatformWriteBytes ( data,lenght );
}

void WebserverSavaDataFinish ( void ) {
	PlatformCloseDataStream();
}

void WebServerSaveDataStart ( unsigned int lenght ) {
	LOG ( FILESYSTEM_LOG,NOTICE_LEVEL,"Saving Data ( %u bytes ) ...\n",lenght );
	writeInt ( 1 );
	writeInt ( 255 );
	writeInt ( 1 );
	writeInt ( 255 );
	writeInt ( lenght );
}

#endif
