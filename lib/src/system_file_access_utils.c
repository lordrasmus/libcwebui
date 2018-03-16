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

#include "intern/system_file_access_utils.h"

static ws_variable* no_ram_cache_files;
static ws_variable* blocked_files;



void init_file_access_utils(void) {



	no_ram_cache_files = newWSArray("no_ram_cache_files");
	blocked_files = newWSArray("blocked_files");

}


int doNotRamCacheFile( WebserverFileInfo *file ){
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

int check_blocked_urls( const char* url ){
	ws_variable *tmp;

	tmp = getWSVariableArrayFirst( blocked_files );
	while (tmp != 0) {
		if( 0 == strcmp( tmp->name , url ) ){
			stopWSVariableArrayIterate( blocked_files );
			return 1;
		}
		tmp = getWSVariableArrayNext( blocked_files );
	}
	stopWSVariableArrayIterate( blocked_files );

	return 0;
}

void VISIBLE_ATTR WebserverAddNoRamCacheFile( const char* url ){
	ws_variable *tmp;

	while( url[0] == '/'){
		url++;
	}
	tmp = getWSVariableArray(no_ram_cache_files, url);

	if (tmp == 0) {
		tmp = addWSVariableArray(no_ram_cache_files, url);
	}
	setWSVariableInt(tmp, 1);
}


void VISIBLE_ATTR WebserverAddBlockedFile( const char* url ){
	ws_variable *tmp;

	while( url[0] == '/'){
		url++;
	}
	tmp = getWSVariableArray( blocked_files, url);

	if (tmp == 0) {
		tmp = addWSVariableArray( blocked_files, url);
	}
	setWSVariableInt(tmp, 1);
}



void copyFilePath(WebserverFileInfo* file, const char* name) {
	file->FilePathLengt = strlen((char*) name);
	if (file->FilePath != 0) {
		WebserverFree( (void*) file->FilePath);
	}
	file->FilePath = ( char*) WebserverMalloc(file->FilePathLengt + 1 );
	strncpy((char*) file->FilePath, (char*) name, file->FilePathLengt);
	((char*)file->FilePath)[file->FilePathLengt] = '\0';
}

void copyURL(WebserverFileInfo* file, const char* url) {

	while( url[0] == '/' ){
		url++;
	}

	file->UrlLengt = strlen((char*) url);
	if (file->Url != 0) {
		WebserverFree( (void*) file->Url);
	}
	file->Url = (char*) WebserverMalloc( file->UrlLengt + 1 );
	strncpy((char*) file->Url, (char*) url, file->UrlLengt);
	((char*)file->Url)[file->UrlLengt] = '\0';
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
		for (i = 0; i < strlen((char*) file->Url) - len; i++){
			ext[i] = tolower(file->Url[i + len]);
		}
		ext[i] = '\0';
	}else{
		ext[0] = '\0';
	}

	/* http://wiki.selfhtml.org/wiki/Referenz:MIME-Typen
	 * http://www.sitepoint.com/web-foundations/mime-types-complete-list/
	 */

	if (0 == strncmp((char*) ext, ".html", 10)){
		file->FileType = FILE_TYPE_HTML;
	}else if (0 == strncmp((char*) ext, ".inc", 10)){
		file->FileType = FILE_TYPE_HTML_INC;
	}else if (0 == strncmp((char*) ext, ".css", 10)){
		file->FileType = FILE_TYPE_CSS;
	}else if (0 == strncmp((char*) ext, ".js", 10)){
		file->FileType = FILE_TYPE_JS;
	}else if (0 == strncmp((char*) ext, ".ico", 10)){
		file->FileType = FILE_TYPE_ICO;
	}else if (0 == strncmp((char*) ext, ".gif", 10)){
		file->FileType = FILE_TYPE_GIF;
	}else if (0 == strncmp((char*) ext, ".jpg", 10)){
		file->FileType = FILE_TYPE_JPG;
	}else if (0 == strncmp((char*) ext, ".png", 10)){
		file->FileType = FILE_TYPE_PNG;
	}else if (0 == strncmp((char*) ext, ".xml", 10)){
		file->FileType = FILE_TYPE_XML;
	}else if (0 == strncmp((char*) ext, ".xsl", 10)){
		file->FileType = FILE_TYPE_XSL;
	}else if (0 == strncmp((char*) ext, ".manifest", 10)){
		file->FileType = FILE_TYPE_MANIFEST;
	}else if (0 == strncmp((char*) ext, ".svg", 10)){
		file->FileType = FILE_TYPE_SVG;
	}else if (0 == strncmp((char*) ext, ".bmp", 10)){
		file->FileType = FILE_TYPE_BMP;
	}else if (0 == strncmp((char*) ext, ".pdf", 10)){
		file->FileType = FILE_TYPE_PDF;
	}else if (0 == strncmp((char*) ext, ".json", 10)){
		file->FileType = FILE_TYPE_JSON;
	}else if (0 == strncmp((char*) ext, ".woff", 10)){
		file->FileType = FILE_TYPE_WOFF;
	}else if (0 == strncmp((char*) ext, ".eot", 10)){
		file->FileType = FILE_TYPE_EOT;
	}else if (0 == strncmp((char*) ext, ".ttf", 10)){
		file->FileType = FILE_TYPE_TTF;
	}else if (0 == strncmp((char*) ext, ".c", 10)){
		file->FileType = FILE_TYPE_C_SRC;
	}else{
		LOG(FILESYSTEM_LOG, NOTICE_LEVEL, 0, "%s is FILE_TYPE_PLAIN", file->Url );
	}
}

#ifndef WEBSERVER_USE_SSL

static uint32_t adler32(const void *buf, uint32_t buflength) {

     const uint8_t *buffer = (const uint8_t*)buf;

     uint32_t s1 = 1;
     uint32_t s2 = 0;

     for (size_t n = 0; n < buflength; n++) {
        s1 = (s1 + buffer[n]) % 65521;
        s2 = (s2 + s1) % 65521;
     }

     return (s2 << 16) | s1;
}

static uint32_t reg32 = 0xffffffff;
 
static uint32_t crc32_byte(char byte){
	uint32_t i;
	uint32_t polynom = 0xEDB88320;

	for (i=0; i<8; ++i){
		if ((reg32 & 1) != (byte & 1)){
			reg32 = (reg32>>1) ^ polynom; 
        }else{
			reg32 >>= 1;
		}
		byte >>= 1;
	}
	return reg32 ^ 0xffffffff;
}

static uint32_t crc32(const char *data, uint32_t len){
  uint32_t i;

  for(i=0; i<len; i++) 
  {
    crc32_byte(data[i]);
  }
  return reg32 ^ 0xffffffff;
}

#endif


void generateEtag(WebserverFileInfo* file) {

#ifndef WEBSERVER_USE_SSL


	if (file->etag == 0) {
		file->etag = (char *) WebserverMalloc( 100 );
	}
	
	uint32_t length = file->DataLenght;
	uint32_t crc   = crc32(   file->Data, file->DataLenght );
	uint32_t adler = adler32( file->Data, file->DataLenght );
	
	file->etagLength = sprintf((char*) file->etag, "%08X%08X%08X", length, crc, adler);

#else

	unsigned char buf[SSL_SHA_DIG_LEN];
	if (file->etag != 0){
		WebserverFree( (void*) file->etag);
	}
	file->etag = (char *) WebserverMalloc ( SSL_SHA_DIG_LEN * 2 + 1 );
	memset( (void*) file->etag ,0,SSL_SHA_DIG_LEN*2+1);

	if (file->RamCached == 1) {
		WebserverSHA1(file->Data, file->DataLenght, buf);
	} else {

#ifdef ENABLE_DEVEL_WARNINGS
		// Datei ist nicht im RAM cache
		#warning hier noch das fs handling einbauen
#endif
		struct sha_context* sha_context;
		unsigned int to_read;
		unsigned long diff;
		FILE_OFFSET pos;
		unsigned char *data;


		if (!PlatformOpenDataReadStream(file->FilePath)){
			return;
		}

		pos = 0;
		sha_context = WebserverSHA1Init();
		data = (unsigned char*) WebserverMalloc ( WRITE_DATA_SIZE );

		PlatformSeekToPosition(0);

		while (1) {
			diff = file->DataLenght - pos;

			if (diff > WRITE_DATA_SIZE) {
				to_read = WRITE_DATA_SIZE;
			} else {
				to_read = diff;
			}

			FILE_OFFSET ret2 = PlatformReadBytes(data, to_read);
			if ( ret2 != to_read ){
				printf("Error: read mismatch %jd != %d\n",ret2,to_read);
			}

			WebserverSHA1Update(sha_context, data, to_read);
			pos += to_read;

			if (pos == file->DataLenght){
				break;
			}

		}
		WebserverSHA1Final(sha_context, buf);
		WebserverSHA1Free(sha_context);
		PlatformCloseDataStream();
		WebserverFree(data);

	}

	convertBinToHexString(buf, SSL_SHA_DIG_LEN, (char*)file->etag, SSL_SHA_DIG_LEN * 2 + 1);
	file->etagLength = strlen( ( char*) file->etag);


#endif

}


WebserverFileInfo *create_empty_file(int pSize)
{

	WebserverFileInfo *file = (WebserverFileInfo *) WebserverMalloc(sizeof(WebserverFileInfo));
	memset(file, 0, sizeof(WebserverFileInfo));

	if( !file ){
		return 0;
	}

	file->RamCached = 1;

	file->FileType = FILE_TYPE_PLAIN;

	file->Data = (unsigned char *) WebserverMalloc(pSize);

	if( !file->Data ){
		return 0;
	}

	file->DataLenght = pSize;

	return file;
}

void free_empty_file(WebserverFileInfo *file)
{
	if(file->Data){
		WebserverFree((char*)file->Data);
	}

	if(file->FilePath){
		WebserverFree((char*)file->FilePath);
	}

	if(file->Url){
		WebserverFree((char*)file->Url);
	}

	WebserverFree(file);
}

/*
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
* */


