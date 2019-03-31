/*

 libCWebUI
 Copyright (C) 2007 - 2016  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

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




