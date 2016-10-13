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

#ifndef _WEBSERVER_SERVER_H_
#define _WEBSERVER_SERVER_H_

#include <webserver.h>


#define HTTP_UNKNOWN_METHOD 0
#define HTTP_POST 1
#define HTTP_GET 2
#define HTTP_OPTIONS 3

#define MULTIPART_FORM_DATA						1
#define APPLICATION_X_WWW_FORM_URLENCODED		2


extern unsigned char *g_lastmodified;  /* Wird fur Binary File gebraucht */


#ifdef __cplusplus
extern "C" {
#endif

int getHttpRequest(socket_info* sock);
int sendHTMLFile(http_request* s, WebserverFileInfo *info);

void sendWebsocketChunk ( socket_info* sock,const unsigned char* text,const unsigned int length );
void printWebsocketChunk ( socket_info* sock,const char *fmt,... );

void sendHeaderChunk(socket_info* sock,const char* text,const unsigned int length);
void sendHeaderChunkEnd(socket_info* sock);
void printHeaderChunk(socket_info* sock,const char *fmt,...);

int  printHTMLChunk(socket_info* sock,const char *fmt,...);
int vprintHTMLChunk(socket_info* sock, const char *fmt, va_list argptr);
void sendHTMLChunk(socket_info* sock,const char* text,const unsigned int length);
void sendHTMLChunkVariable(socket_info* sock,ws_variable* var);

unsigned long getChunkListSize(list_t* liste);

void generateOutputBuffer(socket_info* sock);

int sendFile(http_request* s, WebserverFileInfo *info);

int sendFileNotFound(http_request* s);
int sendAccessDenied(http_request* s);
int sendMethodNotAllowed(socket_info *s);
int sendMethodBadRequest(socket_info *s);
int sendMethodBadRequestLineToLong(socket_info *s);
int sendMethodBadRequestMissingHeaderLines(socket_info *socket);

int checkBuilinSites(http_request* s);


WebserverFileInfo *getFile(char *name);
WebserverFileInfo *getFileById(int id);

void WebserverPrintInfos( void );


#ifdef __cplusplus
}
#endif

#endif
