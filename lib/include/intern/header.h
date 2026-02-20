/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/

#ifndef _HEADER_H_
#define _HEADER_H_

#include "webserver.h"
#include "webserver_api_functions.h"


#define CONTENTTYP_TEXT 		( 1 )
#define CONTENTTYP_OCTETSTREAM 	( 2 )

#ifndef WEBSERVER_MAX_HEADER_LINE_LENGHT
	#error WEBSERVER_MAX_HEADER_LINE_LENGHT not defined
#endif

#define MAX_HEADER_LINE_ERROR 	( -20 )
#define NO_SSL_CONNECTION_ERROR ( -21 )
#define SSL_ACCEPT_OK 			( 2 )




#ifdef __cplusplus
extern "C" {
#endif

int sendHeader(http_request* s, WebserverFileInfo *info, FILE_OFFSET p_lenght);
int sendHeaderNotModified(http_request* s, WebserverFileInfo *info);
int sendHeaderWebsocket(socket_info* sock);
int sendPreflightAllowed(socket_info *sock);

void sendHeaderError(socket_info* sock, char* ErrorMessage,int p_lenght);
void sendHeaderNotFound(http_request* s, int p_lenght);

int analyseHeaderLine(socket_info* sock,char *line,unsigned int length,HttpRequestHeader *header);
int analyseFormDataLine(socket_info* sock, char *line, unsigned int length, HttpRequestHeader *header);

int getHttpRequestHeader(http_request* s);

int ParseHeader(socket_info* sock,HttpRequestHeader* header,char* buffer,unsigned int length, unsigned int* bytes_parsed);

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

