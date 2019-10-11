/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/



#include "webserver.h"

#ifdef DMALLOC
#include <dmalloc/dmalloc.h>
#endif


/* http://www.campin.net/newlogcheck.html */


static void writeToLog ( char* buffer ) {
	#if defined (_WIN32)
		printf ( "LOG : %s",buffer );
	#elif defined ( __GNUC__ )
		printf ( "Webserver: %s",buffer );
		fflush(stdout);
	#else
		#error "writeToLog not handled"
	#endif
}

int last_log_length=0;

void addLog ( LogChannels channel,LogLevels level,char* filename,int fileline,const char* function,int socket,char* text,... ) {
	static char buff[451];/* viel laenger darf eine logzeile nicht sein */
	int len=0,i;
	va_list ap;

	len = snprintf (buff, 450,"%s :  ",function);
	if(len < last_log_length){
		for(i=len;i<last_log_length;i++){
			buff[i] = ' ';
		}
		len = last_log_length;
	}
	if(len > last_log_length){
		last_log_length = len;
	}

	switch ( level ){
		case NOTICE_LEVEL:
			len += snprintf (&buff[len],450-len," NOTICE  : ");
			break;
		case INFO_LEVEL:
			len += snprintf (&buff[len],450-len," INFO    : ");
			break;
		case WARNING_LEVEL:
			len += snprintf (&buff[len],450-len," WARNING : ");
			break;
		case ERROR_LEVEL:
			len += snprintf (&buff[len],450-len," ERROR   : ");
			break;
	}
	len += snprintf (&buff[len],450-len,"( %d ) : ",socket);

	va_start ( ap, text );
	len+= vsnprintf ( &buff[len],450-len, text, ap );
	va_end ( ap );

	switch ( level ){
		case NOTICE_LEVEL:
			break;
		case INFO_LEVEL:
		case WARNING_LEVEL:
		case ERROR_LEVEL:
			len+= snprintf (&buff[len], 450-len,"\n   ->   File: %s, Line :%d",filename,fileline);
			break;
	}

	snprintf ( &buff[len], 450-len,"\n" );
	writeToLog ( buff );
}
