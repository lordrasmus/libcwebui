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
	int ret;

	#ifdef WEBSERVER_USE_SSL
		if(sock->use_ssl == 1){
			ret = WebserverSSLRecvNonBlocking(sock,buf,len,flags);
		}else{
			/* printf("Not an SSL Connection. Reading unencrypted\r\n"); */
			ret = PlatformRecvSocketNonBlocking(sock->socket,buf,len,0);
		}
	#else
		ret = PlatformRecvSocketNonBlocking(sock->socket,buf,len,0);
	#endif

	/* Only a positive return is a byte count - 0 and the negative values are
	 * status codes (CLIENT_NO_MORE_DATA, CLIENT_DISCONNECTED,
	 * CLIENT_UNKNOWN_ERROR). See run_extern_handler() for what the counter is
	 * for; this is the single place every read on a socket_info goes through,
	 * which is why it is kept here and not at the call sites. */
	if (ret > 0) {
		sock->bytes_received += (uint64_t)ret;
	}

	return ret;
}

#ifdef WEBSERVER_USE_SSL


#endif





