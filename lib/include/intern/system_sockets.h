/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

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
	uint32_t getSocketInfoSize(socket_info* sock);

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
