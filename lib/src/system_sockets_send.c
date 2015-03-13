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



SOCKET_SEND_STATUS WebserverSend(socket_info* s, unsigned char *buffer, int len, int flags, int* bytes_send) {
	int ret;

#ifdef WEBSERVER_USE_SSL
	if (s->use_ssl == 1) {
		SOCKET_SEND_STATUS status;
		status = WebserverSSLSendNonBlocking(s, buffer, len, flags, bytes_send);
		return status;
	}
#endif

	if (likely(bytes_send != 0)) *bytes_send = 0;

	ret = PlatformSendSocketNonBlocking(s->socket, buffer, len, flags);
	if (likely(ret == len)) {
		if (likely(bytes_send != 0)) *bytes_send = ret;
		return SOCKET_SEND_NO_MORE_DATA;
	}

	if (ret == CLIENT_SEND_BUFFER_FULL) {
		//LOG(SOCKET_LOG,WARNING_LEVEL,s->socket,"SOCKET_SEND_SEND_BUFFER_FULL    %d of %d send",ret,len);
		return SOCKET_SEND_SEND_BUFFER_FULL;
	}

	if (ret == CLIENT_DISCONNECTED) {
		return SOCKET_SEND_CLIENT_DISCONNECTED;
	}

	if (likely((ret > 0) && (ret < len))) {
		if (likely(bytes_send != 0)) 
			*bytes_send = ret;
		
		#if _WEBSERVER_CONNECTION_DEBUG_ > 0 
			LOG(SOCKET_LOG, WARNING_LEVEL, s->socket, "SOCKET_SEND_NO_MORE_DATA    %d of %d send", ret, len);
		#endif
		
		return SOCKET_SEND_NO_MORE_DATA;
	}

	LOG(CONNECTION_LOG, ERROR_LEVEL, s->socket, "Status %d\n", ret);
	return SOCKET_SEND_UNKNOWN_ERROR;
}


