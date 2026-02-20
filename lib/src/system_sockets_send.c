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




SOCKET_SEND_STATUS WebserverSend(socket_info* sock, const unsigned char *buffer, size_t len, int flags, size_t* bytes_send) {
	ssize_t ret;

#ifdef WEBSERVER_USE_SSL
	if (sock->use_ssl == 1) {
		SOCKET_SEND_STATUS status;
		status = WebserverSSLSendNonBlocking(sock, buffer, len, flags, bytes_send);
		return status;
	}
#endif

	if (likely(bytes_send != 0)){
		*bytes_send = 0;
	}

	ret = PlatformSendSocketNonBlocking(sock->socket, buffer, len, flags);
	if (likely(ret == len)) {
		if (likely(bytes_send != 0)){
			*bytes_send = ret;
		}
		return SOCKET_SEND_NO_MORE_DATA;
	}

	if (ret == CLIENT_SEND_BUFFER_FULL) {
		/* LOG(SOCKET_LOG,WARNING_LEVEL,s->socket,"SOCKET_SEND_SEND_BUFFER_FULL    %d of %d send",ret,len); */
		return SOCKET_SEND_SEND_BUFFER_FULL;
	}

	if (ret == CLIENT_DISCONNECTED) {
		return SOCKET_SEND_CLIENT_DISCONNECTED;
	}

	if (likely((ret > 0) && (ret < len))) {
		if (likely(bytes_send != 0)){
			*bytes_send = ret;
		}

		#if _WEBSERVER_CONNECTION_DEBUG_ > 0
			LOG(SOCKET_LOG, WARNING_LEVEL, sock->socket, "SOCKET_SEND_NO_MORE_DATA    %d of %d send", ret, len);
		#endif

		return SOCKET_SEND_NO_MORE_DATA;
	}

	LOG(CONNECTION_LOG, ERROR_LEVEL, sock->socket, "Status %d\n", ret);
	return SOCKET_SEND_UNKNOWN_ERROR;
}


