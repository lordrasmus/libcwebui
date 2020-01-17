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


int last_log_length=0;

void addLog ( LogChannels channel,LogLevels level,char* filename,int fileline,const char* function,int socket,char* text,... ) {
	char buff[451];/* viel laenger darf eine logzeile nicht sein */
	int len=0,i;
	va_list ap;

	printf( "%s", "Webserver: " );

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
	
	buff[len] = '\0';
	printf( "%s", buff );
	len=0;

	switch ( level ){
		case NOTICE_LEVEL:
			printf(" NOTICE  : ( %d ) : ",socket);
			break;
		case INFO_LEVEL:
			printf(" INFO    : ( %d ) : ",socket);
			break;
		case WARNING_LEVEL:
			printf(" WARNING : ( %d ) : ",socket);
			break;
		case ERROR_LEVEL:
			printf(" ERROR   : ( %d ) : ",socket);
			break;
	}
	
	// Maximal 300 Zeichen vom Variablen teil ausgeben
	va_start ( ap, text );
	len = vsnprintf ( buff ,300, text, ap );
	va_end ( ap );
	if ( len < 300 ){
		buff[len] = '\0';
	}else{
		buff[300] = '\0';
	}
	printf( "%s", buff );
	
	switch ( level ){
		case NOTICE_LEVEL:
			break;
		case INFO_LEVEL:
		case WARNING_LEVEL:
		case ERROR_LEVEL:
			printf( "\n   ->   File: %s, Line :%d",filename,fileline );
			break;
	}
	
	printf( "\n" );
	fflush(stdout);
}
