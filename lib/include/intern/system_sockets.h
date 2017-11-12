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


#ifndef _SYSTEM_SOCKETS_H_
#define _SYSTEM_SOCKETS_H_

typedef enum{
	SOCKET_SEND_NO_MORE_DATA = 1 ,
	SOCKET_SEND_CLIENT_DISCONNECTED = 2,
	SOCKET_SEND_SEND_BUFFER_FULL = 3,
	SOCKET_SEND_SSL_ERROR = 4,
	SOCKET_SEND_UNKNOWN_ERROR = 5,
	SSL_PROTOCOL_ERROR = -22
}SOCKET_SEND_STATUS;

#include "webserver.h"

#ifdef __cplusplus
extern "C" {
#endif

	unsigned long dumpSocketsSize(int *count);
	void	dumpSockets(http_request* s);
	unsigned long getSocketInfoSize(socket_info* sock);

	void	handleServer(socket_info* sock);

	int		WebserverStartConnectionManager( void );
	void	WebserverConnectionManagerStartLoop( void );
	void	WebserverConnectionManagerCloseRequest(socket_info* sock);
	void 	WebserverConnectionManagerCloseConnections(void);
	int		handleWebRequest(socket_info* sock);

	int		WebserverAccept(void);
	int   	WebserverRecv(socket_info* sock,unsigned char *buf, int len, int flags);
	SOCKET_SEND_STATUS 	WebserverSend(socket_info* sock,const unsigned char *buffer, int len, int flags,int* bytes_send);
	int   	WebserverCloseSocket(socket_info* sock);




#ifdef __cplusplus
}
#endif


#endif
