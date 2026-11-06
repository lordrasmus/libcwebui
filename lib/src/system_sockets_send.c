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

	/* Vor dem SSL-Zweig nullen, nicht danach: der SSL-Wrapper schreibt
	 * *bytes_send auf seinen Fehlerpfaden nicht, sodass ein Aufrufer sonst den
	 * Wert des vorherigen Aufrufs weiterverwendet und Bytes als gesendet
	 * zaehlt, die nie rausgingen. */
	if (likely(bytes_send != 0)){
		*bytes_send = 0;
	}

#ifdef WEBSERVER_USE_SSL
	if (sock->use_ssl == 1) {
		SOCKET_SEND_STATUS status;
		/* Always with a counter of our own: progress is what tells the
		 * per-address limit that the connection is in use, and the caller
		 * may not ask for the count */
		size_t sent = 0;
		status = WebserverSSLSendNonBlocking(sock, buffer, len, flags, &sent);
		if (sent > 0) {
			CONN_LIMIT_NOTE_IO(sock);
		}
		if (likely(bytes_send != 0)){
			*bytes_send = sent;
		}
		return status;
	}
#endif


	ret = PlatformSendSocketNonBlocking(sock->socket, buffer, len, flags);
	if (ret > 0) {
		CONN_LIMIT_NOTE_IO(sock);
	}
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
			LOG(SOCKET_LOG, WARNING_LEVEL, sock->socket, "SOCKET_SEND_NO_MORE_DATA    %zd of %zu send", ret, len);
		#endif

		return SOCKET_SEND_NO_MORE_DATA;
	}

	LOG(CONNECTION_LOG, ERROR_LEVEL, sock->socket, "Status %zd\n", ret);
	return SOCKET_SEND_UNKNOWN_ERROR;
}


