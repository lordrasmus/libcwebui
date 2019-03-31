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


#ifndef _PLATFORM_SOCKETS_H_
#define _PLATFORM_SOCKETS_H_

#include "webserver.h"

/********************************************************************
*																	*
*					Socket Funktionen								*
*																	*
********************************************************************/

#define CLIENT_NO_MORE_DATA 	( 0 )
#define CLIENT_DISCONNECTED 	( -1 )
#define CLIENT_UNKNOWN_ERROR 	( -2 )
#define CLIENT_SEND_BUFFER_FULL ( -3 )



#ifdef __cplusplus
extern "C" {
#endif

void	PlatformInitNetwork(void);
void	PlatformEndNetwork(void);

int		PlatformSetNonBlocking(int socket);
int		PlatformSetBlocking(int socket);
int		PlatformClose(int socket);

int		PlatformGetSocket( unsigned short port,int connections);

#ifdef SELECT_HELPER_FUNCS
void	PlatformInitSelect(void);
void	PlatformAddSelectSocket(char write,int socket);
int		PlatformSelectGetActiveSocket(char write,int socket);
int		PlatformSelect(void);
#endif

int		PlatformAccept(socket_info* sock,unsigned int *port);
void	PlatformGetPeerName(socket_info* sock);


int   	PlatformSendSocket(int socket, const unsigned char *buf, SIZE_TYPE len, int flags);
int     PlatformSendSocketNonBlocking(int socket, const unsigned char *buf, SIZE_TYPE len, int flags);
int   	PlatformRecvSocket(int socket, unsigned char *buf, SIZE_TYPE len, int flags);
int     PlatformRecvSocketNonBlocking(int socket, unsigned char *buf, SIZE_TYPE len, int flags);
int     PlatformCloseSocket(int socket);
int		PlatformShutdown(int socket);


#ifdef __cplusplus
}
#endif


#endif

