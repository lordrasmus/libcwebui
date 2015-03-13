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


/**************************************************************
*                                                             *
*                   Netzwerk Operationen                      *
*                                                             *
**************************************************************/

void	PlatformInitNetwork ( void ) {

}

void	PlatformEndNetwork ( void ) {

}

int		PlatformSetNonBlocking ( int socket ){
    return 1;
}

int		PlatformSetBlocking ( int socket ){
    return 1;
}

int		PlatformGetSocket ( unsigned short port,int connections ){
    return 0;
}

#ifdef SELECT_HELPER_FUNCS

void	PlatformInitSelect ( void )
{
}

void	PlatformAddSelectSocket ( char write,int socket )
{
 
}

int		PlatformSelectGetActiveSocket ( char write,int socket ){
	return 0;
}

int		PlatformSelect ( void ){
    return 0;
}

#endif /* SELECT_HELPER_FUNCS */

int		PlatformAccept(socket_info* sock, unsigned int *port){
    
    return 0;
}

int     PlatformSendSocket ( int socket, const unsigned char *buf, SIZE_TYPE len, int flags ){
    
    return 0;
}

int     PlatformSendSocketNonBlocking ( int socket, const unsigned char *buf, SIZE_TYPE len, int flags )
{
    
    return 0;
}

int     PlatformRecvSocket ( int socket, unsigned char *buf, SIZE_TYPE len, int flags )
{
    return 0;
}

int     PlatformRecvSocketNonBlocking ( int socket, unsigned char *buf, SIZE_TYPE len, int flags )
{
    

    return CLIENT_UNKNOWN_ERROR;
}

int     PlatformCloseSocket ( int socket )
{

	return close ( socket );
}




