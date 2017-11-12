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

#include "WebserverConfig.h"


#include "webserver.h"
#include "intern/webserver_ssl.h"


#ifdef DMALLOC
#include <dmalloc/dmalloc.h>
#endif



/* 
 http://tangentsoft.net/wskfaq/articles/bsd-compatibility.html
 http://publib.boulder.ibm.com/infocenter/iseries/v5r3/index.jsp?topic=/rzab6/rzab6xnonblock.htm
 http://www.wangafu.net/~nickm/libevent-book/01_intro.html

 ---------------------------------- SSL --------------------------------------------
 http://www.moserware.com/2009/06/first-few-milliseconds-of-https.html
 http://prefetch.net/articles/debuggingssl.html

*/



int     WebserverRecv(socket_info* sock, unsigned char *buf, int len, int flags)
{
	#ifdef WEBSERVER_USE_SSL
		int l=0;
		if(sock->use_ssl == 1){
			l = WebserverSSLRecvNonBlocking(sock,buf,len,flags);
			return l;
		}else{
			/* printf("Not an SSL Connection. Reading unencrypted\r\n"); */
			return PlatformRecvSocketNonBlocking(sock->socket,buf,len,0);
		}
	#else
		return PlatformRecvSocketNonBlocking(sock->socket,buf,len,0);
	#endif
}

#ifdef WEBSERVER_USE_SSL


#endif





