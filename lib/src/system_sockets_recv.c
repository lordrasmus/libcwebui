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

#include "../WebserverConfig.h"

#ifdef __GNUC__
	/*#include "utils.h"
	#include "system.h"
	#include "server.h"
	#include "cookie.h"
	#include "header.h"	
	#include "webserver_log.h"
	#include "list.h"
	#include "websockets.h"
	#include "webserver_api_functions.h"
	#include "system_sockets_events.h"
	#include "system_sockets.h"

	#include "globals.h"*/
	#include "webserver.h"
	#include "intern/webserver_ssl.h"

#endif




CLIENT_WRITE_DATA_STATUS handleClientWriteData(socket_info* sock);


// http://tangentsoft.net/wskfaq/articles/bsd-compatibility.html
// http://publib.boulder.ibm.com/infocenter/iseries/v5r3/index.jsp?topic=/rzab6/rzab6xnonblock.htm
// http://www.wangafu.net/~nickm/libevent-book/01_intro.html

// ---------------------------------- SSL --------------------------------------------
// http://www.moserware.com/2009/06/first-few-milliseconds-of-https.html
// http://prefetch.net/articles/debuggingssl.html



//int WebserverSLLRecvNonBlocking(socket_info* s, unsigned char *buf, int len, int flags);


int     WebserverRecv(socket_info* s, unsigned char *buf, int len, int flags)
{
	#ifdef WEBSERVER_USE_SSL
		//int ret=0;
		int l=0;
		if(s->use_ssl == 1){
			l = WebserverSSLRecvNonBlocking(s,buf,len,flags);
			return l;
		}else{
			//printf("Not an SSL Connection. Reading unencrypted\r\n");
			return PlatformRecvSocketNonBlocking(s->socket,buf,len,0);
		}
	#else
		return PlatformRecvSocketNonBlocking(s->socket,buf,len,0);
	#endif
}

#ifdef WEBSERVER_USE_SSL


#endif





