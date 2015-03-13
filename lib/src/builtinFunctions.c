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



#ifdef _WEBSERVER_MEMORY_DEBUG_
    extern int print_blocks_now;
#endif

extern unsigned long allocated;
extern unsigned long allocated_max;


DEFINE_FUNCTION_INT( compiler ) {
#ifdef __INTEL_COMPILER
    unsigned int t1,patch,sversion,version;
    t1 =  __INTEL_COMPILER;
    patch = t1 % 10;
    t1 = t1 / 10;
    sversion = t1 % 10;
    t1 = t1 / 10;
    version = t1 ;

    printHTMLChunk ( s->socket ,"Intel Compiler %d.%d.%d",version,sversion,patch );
    return;
#endif

#ifdef __GNUC__
#if __GNUC__ > 2
    printHTMLChunk ( s->socket ,"GCC %d.%d.%d",__GNUC__,__GNUC_MINOR__,__GNUC_PATCHLEVEL__ );
    return;
#else
     printHTMLChunk ( s->socket ,"GCC %d.%d",__GNUC__,__GNUC_MINOR__ );
     return;
#endif
#endif

	 // https://en.wikipedia.org/wiki/Microsoft_Visual_C%2B%2B
	 // https://dev.to/yumetodo/list-of-mscver-and-mscfullver-8nd

#ifdef _MSC_FULL_VER
#if _MSC_FULL_VER == 191627027
	 printHTMLChunk(s->socket, "Microsoft Visual C++ 2017 (15.9.7)"); return;
#endif
	 printHTMLChunk(s->socket, "_MSC_FULL_VER %d", _MSC_FULL_VER);
#endif

#ifdef _MSC_VER
#if _MSC_VER == 600
    printHTMLChunk ( s->socket ,"Microsoft C Compiler version 6.0" );
#elif _MSC_VER == 700
    printHTMLChunk ( s->socket ,"Microsoft C/C++ compiler version 7.0" );
#elif _MSC_VER == 800
    printHTMLChunk ( s->socket ,"Microsoft Visual C++, 32-bit, version 1.0" );
#elif _MSC_VER == 900
    printHTMLChunk ( s->socket ,"Microsoft Visual C++, 32-bit, version 2.0" );
#elif _MSC_VER == 1000
    printHTMLChunk ( s->socket ,"Microsoft Visual C++, 32-bit, version 4.0" );
#elif _MSC_VER == 1100
    printHTMLChunk ( s->socket ,"Microsoft Visual C++, 32-bit, version 5.0" );
#elif _MSC_VER == 1200
    printHTMLChunk ( s->socket ,"Microsoft Visual C++, 32-bit, version 6.0" );
#elif _MSC_VER == 1300
    printHTMLChunk ( s->socket ,"Microsoft Visual C++ 2002 (7.0)" );
#elif _MSC_VER == 1310
    printHTMLChunk ( s->socket ,"Microsoft Visual C++ 2003 (7.1)" );
#elif _MSC_VER == 1400
    printHTMLChunk ( s->socket ,"Microsoft Visual C++ 2005 (8.0)" );
#elif _MSC_VER == 1500
    printHTMLChunk ( s->socket ,"Microsoft Visual C++ 2008 (9.0)" );
#elif _MSC_VER == 1913
	 printHTMLChunk(s->socket, "Microsoft Visual C++ 2017 (14.12)"); 
#elif _MSC_VER == 1916
	 printHTMLChunk(s->socket, "Visual Studio 2017 version ( 15.9 )"); return;
#else
    printHTMLChunk ( s->socket ,"Microsoft %d",_MSC_VER );
#endif
	return;
#endif
    printHTMLChunk ( s->socket ,"Unknown Compiler" );
}

static int miniFunctions ( http_request* s,char* buffer ) {
    if ( !strncmp ( buffer,"ssl_avaible",11 ) ) {
#ifdef WEBSERVER_USE_SSL
        printHTMLChunk ( s->socket ,"AVAILABLE" );
#else
        printHTMLChunk ( s->socket ,"NOT AVAIBLE" );
#endif
        return 1;
    }
    if ( !strncmp ( buffer,"64bit_avaible",13 ) ) {
#ifdef __INTEL_COMPILER
#ifdef _WIN64
        printHTMLChunk ( s->socket ,"AVAILABLE" );
        return 1;
#else
#ifdef __LP64__
        printHTMLChunk ( s->socket ,"AVAILABLE" );
        return 1;
#else
        printHTMLChunk ( s->socket ,"NOT AVAILABLE" );
        return 1;
#endif
#endif
#endif

#ifdef __GNUC__
#ifdef __LP64__
        printHTMLChunk ( s->socket ,"AVAILABLE" );
#else
        printHTMLChunk ( s->socket ,"NOT AVAILABLE" );
#endif
#endif



#ifdef _MSC_VER
#ifdef _WIN64
        printHTMLChunk ( s->socket ,"AVAILABLE" );
#else
        printHTMLChunk ( s->socket ,"NOT AVAILABLE" );
#endif
#endif
        return 1;
    }
    return 0;
}

static int sessionFunctions ( http_request* s,char* buffer ) {
    if ( !strncmp ( buffer,"startSession",12 ) ) {
#ifdef WEBSERVER_USE_SESSIONS
#ifdef _WEBSERVER_SESSION_DEBUG_
        WebServerPrintf ( "builtinFunction: Start Session\r\n" );
#endif
        restoreSession ( s ,1, 1 );

#else
        WebServerPrintf ( "builtinFunction: Start Session not AVAILABLE\r\n" );
#endif
        return 1;
    }
    return 0;
}

DEFINE_FUNCTION_INT( memoryInfos ){
    printHTMLChunk ( s->socket,"<font bold size=3>Allocated Memory</font>" );
    printHTMLChunk ( s->socket,"<table><th>Typ<th>Bytes" );
    printHTMLChunk ( s->socket,"<tr><td>CUR<td>%ld",allocated );
    printHTMLChunk ( s->socket,"<tr><td>MAX<td>%ld",allocated_max );
    printHTMLChunk ( s->socket,"</table>" );
}

static int memoryInfosDetail ( http_request* s ) {
    int all_file_size=0;
    int size=0;
    int session_store_count=0;
    unsigned long all_session_store_size;
	unsigned long sockets_size;
	int sockets;
    int count = 0;

    all_file_size = getLoadedFilesSize(&count);
	all_session_store_size = dumpSessionsSize(&session_store_count);
	sockets_size = dumpSocketsSize(&sockets);

    size+=all_file_size + all_session_store_size + sockets_size;

    printHTMLChunk ( s->socket,"<tr><td>Files<td>%d<td>%d",count,all_file_size );
    printHTMLChunk ( s->socket,"<tr><td>SessionStores<td>%d<td>%ld",session_store_count,all_session_store_size );
	printHTMLChunk ( s->socket,"<tr><td>Sockets<td>%d<td>%ld",sockets,sockets_size);
    printHTMLChunk ( s->socket,"<tr><td>MemorySize<td>%d",size );
    printHTMLChunk ( s->socket,"<tr><td>AllocatedMemory<td>%ld",allocated );
    printHTMLChunk ( s->socket,"<tr><td>AllocatedMemory MAX<td>%ld",allocated_max );
    printHTMLChunk ( s->socket,"" );
#ifdef _WEBSERVER_MEMORY_DEBUG_
    print_blocks_now = 1;
#endif
    return 0;
}





static void getServerLink ( http_request* s ) {
#ifdef WEBSERVER_USE_IPV6
    char mybuf[INET6_ADDRSTRLEN];
    if ( s->socket->v6_client == 1 ) {
        PlatformGetIPv6 ( mybuf );
        if ( sock->use_ssl == 1 )
        	printHTMLChunk ( s->socket,"https://[%s]:%d",mybuf,webserver_ssl_port );
        else
        	printHTMLChunk ( s->socket,"http://[%s]:%d",mybuf,webserver_port );
    } else {
        PlatformGetIP ( mybuf );
        if ( sock->use_ssl == 1 )
        	printHTMLChunk ( s->socket,"https://%s:%d",mybuf,webserver_ssl_port );
        else
        	printHTMLChunk ( s->socket,"http://%s:%d",mybuf,webserver_port );
    }
#else

#ifdef WEBSERVER_USE_SSL
	if ( s->socket->use_ssl == 1 ) {
        printHTMLChunk ( s->socket , "https://" );
    } else {
        printHTMLChunk ( s->socket , "http://" );
    }
    printHTMLChunk ( s->socket,"%s",getConfigText( "server_ip") );
    if ( s->socket->use_ssl == 1 ) {
        printHTMLChunk ( s->socket ,":%d",getConfigInt("ssl_port") );
    } else {
        printHTMLChunk ( s->socket ,":%d",getConfigInt("port") );
    }
#else

	printHTMLChunk(s->socket, "http://");
	printHTMLChunk(s->socket, "%s", getConfigText("server_ip"));
	printHTMLChunk(s->socket, ":%d", getConfigInt("port"));
	
#endif
#endif

}

static void getServerLinkStd ( http_request* s ) {
#ifdef WEBSERVER_USE_IPV6
    //char mybuf[INET6_ADDRSTRLEN];
    if ( s->sock->v6_client == 1 ) {
        PlatformGetIPv6 ( mybuf );
        printHTMLChunk ( s->socket , "http://[%s]:%d",mybuf,globals.config.port );
    } else {
        PlatformGetIP ( mybuf );
        printHTMLChunk ( s->socket , "http://%s:%d",mybuf,globals.config.port );
    }
#else
    printHTMLChunk ( s->socket , "http://" );
    if ( s->header->Host != 0 ) {
		char *tmp = strchr( s->header->Host, ':' );
		if( tmp ){
			char host_tmp[1001];
			strncpy( host_tmp, s->header->Host , 1000 );
			tmp =strchr( host_tmp, ':' );
			*tmp = '\0';
			printHTMLChunk ( s->socket,"%s",host_tmp );
		}else{
			printHTMLChunk ( s->socket,"%s",s->header->Host );
		}
		printHTMLChunk ( s->socket ,":%d",getConfigInt("port"));
    } else {
        printHTMLChunk ( s->socket,"%s",getConfigText( "server_ip" )  );
        printHTMLChunk ( s->socket ,":%d",getConfigInt("port"));
    }
#endif
}

static void getServerLinkSSL ( http_request* s ) {
#ifdef WEBSERVER_USE_IPV6
    //static char mybuf[INET6_ADDRSTRLEN];
    if ( s->socket->v6_client == 1 ) {
        PlatformGetIPv6 ( mybuf );
        printHTMLChunk ( s->socket , "https://[%s]:%d",mybuf,globals.config.ssl_port );
    } else {
        PlatformGetIP ( mybuf );
        printHTMLChunk ( s->socket , "https://%s:%d",mybuf,globals.config.ssl_port );
    }
#else
    printHTMLChunk ( s->socket , "https://" );
     if ( s->header->Host != 0 ) {
		char *tmp = strchr( s->header->Host, ':' );
		if( tmp ){
			char host_tmp[1000];
			strncpy( host_tmp, s->header->Host , 999 );
			tmp =strchr( host_tmp, ':' );
			*tmp = '\0';
			printHTMLChunk ( s->socket,"%s",host_tmp );
		}else{
			printHTMLChunk ( s->socket,"%s",s->header->Host );
		}
		printHTMLChunk ( s->socket ,":%d",getConfigInt("ssl_port"));
    } else {
        printHTMLChunk ( s->socket,"%s",getConfigText( "server_ip" )  );
        printHTMLChunk ( s->socket ,":%d",getConfigInt("ssl_port"));
    }

#endif
}

/*DEFINE_FUNCTION_INT( ip ){
	printHTMLChunk(s->socket,"%s",getConfigText( "server_ip") );
}*/

DEFINE_FUNCTION_INT( server_name ){
	printHTMLChunk ( s->socket,"%s",SERVER_NAME );
}

DEFINE_FUNCTION_INT( session_timeout ){
	printHTMLChunk ( s->socket,"%d",getConfigInt("session_timeout") );
}

DEFINE_FUNCTION_INT( host ){
	printHTMLChunk ( s->socket,"%s",s->header->Host );
}

DEFINE_FUNCTION_INT( host_name ){
	printHTMLChunk ( s->socket,"%s",s->header->HostName );
}

DEFINE_FUNCTION_INT( build_time ){
	 printHTMLChunk ( s->socket ,"%s %s", __DATE__,__TIME__ );
}

DEFINE_FUNCTION_INT( server_port ){
	 printHTMLChunk ( s->socket,"%d",getConfigInt("port") );
}

DEFINE_FUNCTION_INT( server_ssl_port ){
	 printHTMLChunk ( s->socket,"%d",getConfigInt("ssl_port") );
}

void register_internal_funcs( void ) {

#ifndef _MSC_VER
	REGISTER_FUNCTION_INT( dump_render_vars );
#endif
	REGISTER_LOCAL_FUNCTION_INT( compiler );
	REGISTER_LOCAL_FUNCTION_INT( memoryInfos );
/*	REGISTER_FUNCTION( ip ); */
	REGISTER_LOCAL_FUNCTION_INT( server_name );
	REGISTER_LOCAL_FUNCTION_INT( session_timeout );
	REGISTER_LOCAL_FUNCTION_INT( host );
	REGISTER_LOCAL_FUNCTION_INT( host_name );
	REGISTER_LOCAL_FUNCTION_INT( build_time );
	REGISTER_LOCAL_FUNCTION_INT( server_port );
	REGISTER_LOCAL_FUNCTION_INT( server_ssl_port );

}

#define CheckFunktion(a) if(0==strncmp((char*)func->parameter[0].text,a,strlen(a))) if(func->parameter[0].text[strlen(a)]=='\0')

int builtinFunction ( http_request* s,FUNCTION_PARAS* func ) {

    CheckFunktion ( "memoryInfosDetail" ) {
        memoryInfosDetail ( s );
        return 0;
    }

#ifdef WEBSERVER_USE_IPV6
    CheckFunktion ( "ipv6" ) {
		char mybuf[200];
        PlatformGetIPv6 ( ( char* ) mybuf );
        printHTMLChunk(s->socket, "[%s]",mybuf );
        return 0;
    }
#endif


    CheckFunktion ( "link" ) {
        getServerLink ( s );
        return 0;
    }
    CheckFunktion ( "link_std" ) {
        getServerLinkStd ( s );
        return 0;
    }
    CheckFunktion ( "link_ssl" ) {
        getServerLinkSSL ( s );
        return 0;
    }
    CheckFunktion ( "registeredFunctionsDetail" ) {
        printRegisteredFunctions ( s );
        return 0;
    }
    CheckFunktion ( "registeredConditionsDetail" ) {
        printRegisteredConditions ( s );
        return 0;
    }
    CheckFunktion ( "registeredPlugins" ) {
        printRegisteredPlugins ( s );
        return 0;
    }
    CheckFunktion ( "printLoadedFiles" ) {
    	dumpLoadedFiles ( s );
        return 0;
    }
	CheckFunktion ( "dumpGlobals" ){
		dumpGlobals(s);
		return 0;
	}
	CheckFunktion ( "dumpSessions" ){
		dumpSessions(s);
		return 0;
	}
	CheckFunktion ( "dumpSockets" ){
		dumpSockets(s);
		return 0;
	}
	CheckFunktion ( "dumpSessionStore" ) {
		dumpSession ( s );
        return 0;
    }
    if ( miniFunctions ( s,func->parameter[0].text ) ){
        return 0;
    }

    if ( sessionFunctions ( s,func->parameter[0].text ) ){
        return 0;
    }

    printHTMLChunk ( s->socket ,"Unbekannt :%s\n",func->parameter[0].text );
    return 0;
}



