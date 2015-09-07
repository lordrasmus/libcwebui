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


#ifndef _WEBSERVER_LOG_H_
#define _WEBSERVER_LOG_H_

#include "webserver.h"

typedef enum {
	MESSAGE_LOG,
	CONNECTION_LOG,
	HEADER_PARSER_LOG,
	TEMPLATE_LOG,
	PLATFORM_LOG,
	SOCKET_LOG,
	HANDLER_LOG,
	FILESYSTEM_LOG,
	CACHE_LOG,
	SSL_LOG,
	WEBSOCKET_LOG,
	VARIABLE_LOG
}LogChannels;

typedef enum {
	NOTICE_LEVEL,
	INFO_LEVEL,
	WARNING_LEVEL,
	ERROR_LEVEL
}LogLevels;

typedef struct{
	const char* file;
	int line;
	char* text;
} FireLogger;

#ifdef _MSC_VER
	#define FireLog(a,b,...) addFirePHPLog(a,__FILE__, __LINE__,b, __VA_ARGS__)
#else

	#ifdef __GNUC__
		#define FireLog(a,ARGS...) addFirePHPLog(a,(char*)__FILE__, __LINE__, ARGS)
	#else
		#error "Compiler nicht erkannt"
	#endif

#endif

#ifdef __cplusplus
extern "C" {
#endif

void addFirePHPLog(http_request* s,char* filename,int fileline,char* text,...);
void vaddFirePHPLog ( http_request* s,const char* filename,int fileline,const char* text, va_list ap );



#ifdef _MSC_VER
	#define LOG(a,b,c,...) addLog(a,b,__FILE__, __LINE__,__FUNCTION__,c, __VA_ARGS__)
#endif
#ifdef __GNUC__
	#if __GNUC__ > 2
		#define LOG(a,b,c,d,ARGS...) addLog(a,b,(char*)__BASE_FILE__, __LINE__,__FUNCTION__,c,(char*)d, ARGS)
	#else
		#define LOG(a,b,c,d,ARGS...)
	#endif
#endif



void addLog(LogChannels channel,LogLevels level,char* filename,int fileline,const char* function,int socket,char* text,...);

#ifdef __cplusplus
}
#endif


#endif

