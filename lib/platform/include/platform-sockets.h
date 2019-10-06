/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

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

#ifdef USE_SELECT
    #define SELECT_HELPER_FUNCS
#endif


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

