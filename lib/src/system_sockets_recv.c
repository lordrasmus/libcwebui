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





