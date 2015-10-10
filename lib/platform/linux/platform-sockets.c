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

#include <stdio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "webserver.h"


#ifdef DMALLOC
#include <dmalloc/dmalloc.h>
#endif


fd_set read_fds;
fd_set write_fds;
int select_listen_sockets;
int select_listen_max;

/**************************************************************
*                                                             *
*                   Netzwerk Operationen                      *
*                                                             *
**************************************************************/

void	PlatformInitNetwork ( void ) {}

void	PlatformEndNetwork ( void ) {}

int		PlatformSetNonBlocking ( int socket )
{
    long arg;
    /* Set non-blocking */
    if ( ( arg = fcntl ( socket, F_GETFL, NULL ) ) < 0 )
    {
        LOG ( CONNECTION_LOG,ERROR_LEVEL,socket, "Error fcntl(..., F_GETFL) (%s)", strerror ( errno ) );
        exit ( 0 );
    }
    arg |= O_NONBLOCK;
    if ( fcntl ( socket, F_SETFL, arg ) < 0 )
    {
        LOG ( CONNECTION_LOG,ERROR_LEVEL,socket, "Error fcntl(..., F_SETFL) (%s)", strerror ( errno ) );
        exit ( 0 );
    }
    return 1;
}

int		PlatformSetBlocking ( int socket )
{
    long arg;
    /* Set blocking */
    if ( ( arg = fcntl ( socket, F_GETFL, NULL ) ) < 0 )
    {
        LOG ( CONNECTION_LOG,ERROR_LEVEL,socket, "Error fcntl(..., F_GETFL) (%s)", strerror ( errno ) );
        exit ( 0 );
    }
    arg &= ~O_NONBLOCK;
    if ( fcntl ( socket, F_SETFL, arg ) < 0 )
    {
        LOG ( CONNECTION_LOG,ERROR_LEVEL,socket,"Error fcntl(..., F_SETFL) (%s)", strerror ( errno ) );
        exit ( 0 );
    }
    return 1;
}

int		PlatformGetSocket ( unsigned short port,int connections )
{
    int on;

#ifdef WEBSERVER_USE_IPV6
    struct sockaddr_in6 *addr = ( struct sockaddr_in6 * ) malloc ( sizeof ( struct sockaddr_in6 ) );
    int s = socket ( PF_INET6, SOCK_STREAM, 0 );
#else
    struct sockaddr_in *addr = ( struct sockaddr_in * ) malloc ( sizeof ( struct sockaddr_in ) );
    int s = socket ( PF_INET, SOCK_STREAM, 0 );
#endif

    if ( s == -1 )
    {
        LOG ( CONNECTION_LOG,ERROR_LEVEL,0,"socket() failed","" );
        return -1;
    }


#ifdef WEBSERVER_USE_IPV6
    addr->sin6_addr = in6addr_any;
    addr->sin6_port = htons ( port );
    addr->sin6_family = AF_INET6;
#else
    addr->sin_addr.s_addr = INADDR_ANY;
    addr->sin_port = (uint16_t)	htons ( port );
    addr->sin_family = AF_INET;
#endif
    on = 1;
   /* setsockopt ( s, SOL_SOCKET, SO_REUSEPORT, ( char* ) &on, sizeof ( on ) ); */
    setsockopt ( s, SOL_SOCKET, SO_REUSEADDR, ( char* ) &on, sizeof ( on ) );


#ifdef WEBSERVER_USE_IPV6
    if ( bind ( s, ( struct sockaddr* ) addr, sizeof ( struct sockaddr_storage ) ) == -1 )
#else
    if ( bind ( s, ( struct sockaddr* ) addr, sizeof ( struct sockaddr_in ) ) == -1 )
#endif
    {
        LOG ( CONNECTION_LOG,ERROR_LEVEL,0,"bind() failed","" );
        free(addr);
        return -2;
    }

    if ( listen ( s, connections ) == -1 )
    {
        LOG ( CONNECTION_LOG,ERROR_LEVEL,0,"listen() failed","" );
        free(addr);
        return -3;
    }
    free(addr);
    return s;
}

#ifdef SELECT_HELPER_FUNCS

void	PlatformInitSelect ( void )
{
    FD_ZERO ( &read_fds );
    FD_ZERO ( &write_fds );
    select_listen_sockets = 0;
    select_listen_max = 0;
}

void	PlatformAddSelectSocket ( char write,int socket )
{
    if ( write==0 )
        FD_SET ( socket, &read_fds );
    else
        FD_SET ( socket, &write_fds );
    select_listen_sockets++;
    if ( socket > select_listen_max )
        select_listen_max = socket;
}

int		PlatformSelectGetActiveSocket ( char write,int socket )
{
    if ( write==0 )
        return FD_ISSET ( socket,&read_fds );
    else
        return FD_ISSET ( socket,&write_fds );
}

int		PlatformSelect ( void )
{
    int ret;

    /*
	printf("Warte auf select .. ");
    fflush(stdout);
	*/

    ret = select ( select_listen_max + 1, &read_fds, &write_fds, NULL, NULL );

    if ( ret == -1 )
    {
        LOG ( CONNECTION_LOG,ERROR_LEVEL,0,"select error %d",ret );
    }
    else
    {
        /*printf("OK\n"); */
    }

    return ret;
}

#endif /* SELECT_HELPER_FUNCS */

int		PlatformAccept(socket_info* sock, unsigned int *port)
{
    int ret=0;
	char* ret2;

#ifdef WEBSERVER_USE_IPV6
    struct sockaddr_in6 clientaddr;
#else
    struct sockaddr_in clientaddr;
#endif

#ifdef WEBSERVER_USE_IPV6
    sock->v6_client=1;
#endif

    socklen_t addrlen=sizeof ( clientaddr );

    ret = accept ( sock->socket, NULL, 0 );
    if ( ret > 0 )
    {
        getpeername ( ret, ( struct sockaddr * ) &clientaddr, &addrlen );
		*port = ntohl(clientaddr.sin_port);
#ifdef WEBSERVER_USE_IPV6
        if ( inet_ntop ( AF_INET6, &clientaddr.sin6_addr, sock->ip_str, sizeof ( sock->ip_str ) ) )
        {
            /*
		if(0==IN6_IS_ADDR_V4MAPPED(&clientaddr))	ipv6 = 0;
            	if(0==IN6_IS_ADDR_V4COMPAT(&clientaddr))	ipv6 = 0;
	    */
            if ( clientaddr.sin6_addr.__in6_u.__u6_addr32[0] == 0 )
            {
                if ( clientaddr.sin6_addr.__in6_u.__u6_addr32[1] == 0 )
                {
                    if ( clientaddr.sin6_addr.__in6_u.__u6_addr32[2] == 0xFFFF0000 )
                    {
                        sock->v6_client = 0;
                    }
                }
            }
            /*if(sock->v6_client == 0){
            	printf("IPv4 CLient ");
            }else{
            	printf("IPv6 CLient ");
            }*/
#else
		*port = clientaddr.sin_port;
		ret2 = (char*)inet_ntop ( AF_INET, &clientaddr.sin_addr, sock->client_ip_str, sizeof ( sock->client_ip_str ) );
        if ( ret2 != 0  )
        {
#endif
            /*printf("Client address is %s\n", sock->ip_str);
            printf("Client port is %d\n", ntohs(clientaddr.sin6_port));*/
        }
    }

    return ret;
}

int     PlatformSendSocket ( int socket, const unsigned char *buf, SIZE_TYPE len, int flags )
{
    int ret,ret2;
	SIZE_TYPE send_bytes = 0;

	if ( len > INT_MAX ){
		LOG ( CONNECTION_LOG,ERROR_LEVEL,socket,"len > INT_MAX ( %d / %d ) ",len,INT_MAX );
        return CLIENT_UNKNOWN_ERROR;
	}

    while ( 1 )
    {
        ret = send ( socket,buf,len,flags );
        if ( ret < 0 )
        {
            ret2 = errno;
            if ( ( ret2 == EWOULDBLOCK ) || ( ret2 == EAGAIN ) )
            {
                /*tx_thread_sleep(100); */
                continue;
            }
            else
            {
                break;
            }
        }
        send_bytes += (SIZE_TYPE)ret;
        if ( send_bytes == len )
            break;
    }
    return ret;
}

int     PlatformSendSocketNonBlocking ( int socket, const unsigned char *buf, SIZE_TYPE len, int flags )
{
    int ret,ret2;
    ret = send ( socket,buf,len,flags );
    if ( ret == -1 )
    {
        ret2 = errno;
        if ( ( ret2 == EWOULDBLOCK ) || ( ret2 == EAGAIN ) )
        {
            /*LOG ( CONNECTION_LOG,ERROR_LEVEL,socket,"EWOULDBLOCK | EAGAIN","" );*/
            return CLIENT_SEND_BUFFER_FULL;
        }
        if ( errno == ECONNRESET )
        {
            LOG ( CONNECTION_LOG,ERROR_LEVEL,socket,"ECONNRESET","" );
            return CLIENT_DISCONNECTED;
        }
        if ( errno == EPIPE )
        {
            LOG ( CONNECTION_LOG,ERROR_LEVEL,socket,"EPIPE","" );
            return CLIENT_DISCONNECTED;
        }

        LOG ( CONNECTION_LOG,ERROR_LEVEL,socket,"Unhandled Error %d",ret2 );
        return ret;
    }
    return ret;
}

int     PlatformRecvSocket ( int socket, unsigned char *buf, SIZE_TYPE len, int flags )
{
    return recv ( socket,buf,len,flags );
}

int     PlatformRecvSocketNonBlocking ( int socket, unsigned char *buf, SIZE_TYPE len, int flags )
{
    SIZE_TYPE len2 = 0;
    int ret;

    if ( len > INT_MAX ){
		LOG ( CONNECTION_LOG,ERROR_LEVEL,socket,"len > INT_MAX ( %d / %d ) ",len,INT_MAX );
        return CLIENT_UNKNOWN_ERROR;
	}

    while ( 1 )
    {
        ret = recv ( socket,&buf[len2],len-len2,flags );
        if ( ret == 0 )
        {
            return CLIENT_DISCONNECTED;
        }
        if ( ret < 0 )
        {
            if ( ( errno  == EAGAIN ) || ( errno  == EWOULDBLOCK ) )
            {
                if ( len2 == 0 )
                {
                    return CLIENT_NO_MORE_DATA;
                }
                else
                {
                    return (int)len2;
                }
            }

            if ( errno == ECONNRESET )
            {
                return CLIENT_DISCONNECTED;
            }

            LOG ( CONNECTION_LOG,ERROR_LEVEL,socket,"readSocket Error","" );
            LOG ( CONNECTION_LOG,ERROR_LEVEL,socket,"bytes : %d",ret );
            return CLIENT_UNKNOWN_ERROR;
        }

        len2 += (SIZE_TYPE)ret;
        if ( len == len2 ){
            return (int)len2;
        }
    }


    return CLIENT_UNKNOWN_ERROR;
}

int     PlatformCloseSocket ( int socket )
{

	return close ( socket );
}




