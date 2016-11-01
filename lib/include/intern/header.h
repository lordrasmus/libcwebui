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

#ifndef _HEADER_H_
#define _HEADER_H_

#include "webserver.h"
#include "webserver_api_functions.h"


#define CONTENTTYP_TEXT 1
#define CONTENTTYP_OCTETSTREAM 2

#ifndef WEBSERVER_MAX_HEADER_LINE_LENGHT
	#error WEBSERVER_MAX_HEADER_LINE_LENGHT not defined
#endif

#define MAX_HEADER_LINE_ERROR -20
#define NO_SSL_CONNECTION_ERROR -21
#define SSL_ACCEPT_OK 2




#ifdef __cplusplus
extern "C" {
#endif

int sendHeader(http_request* s, WebserverFileInfo *info,int p_lenght);
int sendHeaderNotModified(http_request* s, WebserverFileInfo *info);
int sendHeaderWebsocket(socket_info* sock);
int sendPreflightAllowed(socket_info *sock);

void sendHeaderError(socket_info* socket, char* ErrorMessage,int p_lenght);
void sendHeaderNotFound(http_request* s, int p_lenght);

int analyseHeaderLine(socket_info* socket,char *line,unsigned int length,HttpRequestHeader *header);
int analyseFormDataLine(socket_info* sock, char *line, unsigned int length, HttpRequestHeader *header);

int getHttpRequestHeader(http_request* s);

int ParseHeader(socket_info* socket,HttpRequestHeader* header,char* buffer,unsigned int length, unsigned int* bytes_parsed);

void clearHeader(http_request *s);

void freeHeader(http_request *s);

void printHeaderInfo(HttpRequestHeader *header);

ws_variable* getParameter(http_request* s,const char* name);

bool checkHeaderComplete(HttpRequestHeader* header);

void setCORS_Handler( cors_handler handler );
char* get_cors_type_name( CORS_HEADER_TYPES type );

#ifdef __cplusplus
}
#endif

#endif

