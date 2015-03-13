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

#include "stdafx.h"


#ifdef __GNUC__
	#include "webserver.h"
#endif

//http://www.campin.net/newlogcheck.html


void vaddFirePHPLog ( http_request* s,const char* filename,int fileline,const char* text, va_list ap ) {
    static char buff[450];
    int len=0;
    FireLogger *fl=0;

    len+= vsnprintf ( ( char* ) buff ,450, ( char* ) text, ap );

	fl = (FireLogger*) WebserverMalloc( sizeof(FireLogger) );
	fl->file = filename;
	fl->line = fileline;
	fl->text = ( char* ) WebserverMalloc ( len + 1 );
	Webserver_strncpy ( fl->text,len+1,buff,len );

	ws_list_append(&s->socket->firephplogs,fl);
}

void addFirePHPLog ( http_request* s,char* filename,int fileline,char* text,... ) {
	va_list ap;
	va_start ( ap, text );
	vaddFirePHPLog(s,filename,fileline,text,ap);
	va_end ( ap );
}

/*
void clearFirePHPLogElement ( void* p ) {
    WebserverFree ( p );
}

void clearFirePHPLog ( HttpRequestHeader* header ) {
    if ( header->firephplogs!=0 ) {
        //freeList(header->firephplogs,clearFirePHPLogElement);
        deleteList ( header->firephplogs,clearFirePHPLogElement );
        header->firephplogs=0;
    }
}*/

void writeToLog ( char* buffer ) {
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
	static char buff[450];// viel l�nger darf eine logzeile nicht sein
	int len=0,i;
	va_list ap;

	//len = snprintf (buff, 450,"%s : %s %d  ",function,filename,fileline);
	len = snprintf (buff, 450,"%s :  ",function);
	if(len < last_log_length){
		for(i=len;i<last_log_length;i++){
			strncat(buff," ",450);
		}
		len = last_log_length;
	}
	if(len > last_log_length)
		last_log_length = len;

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
	len+= vsnprintf ( &buff[len],450-len, ( char* ) text, ap );
	va_end ( ap );

	switch ( level ){
		case NOTICE_LEVEL:
			break;
		case INFO_LEVEL:
		case WARNING_LEVEL:
		case ERROR_LEVEL:
			len+= snprintf (&buff[len], 450,"\n   ->   File: %s, Line :%d",filename,fileline);
			break;
	}

	len+= snprintf ( &buff[len], 450-len,"\n" );
	//printf("%s",buff);
	writeToLog ( buff );
}
