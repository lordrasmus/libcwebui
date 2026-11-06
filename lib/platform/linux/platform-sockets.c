/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/

#ifdef __CDT_PARSER__
	#define __BASE_FILE__ base
#endif


#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>


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

void	PlatformInitNetwork ( void ) {
	// Intentionally unimplemented... function needed for winsock
}

void	PlatformEndNetwork ( void ) {
	// Intentionally unimplemented... function needed for winsock
}

int		PlatformSetNonBlocking ( int socket )
{
    long arg = fcntl ( socket, F_GETFL, NULL )  ;
    /* Set non-blocking */
    if ( arg < 0 )
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
    long arg = fcntl ( socket, F_GETFL, NULL );
    /* Set blocking */
    if ( arg < 0 )
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
    struct sockaddr_in6 *addr = ( struct sockaddr_in6 * ) WebserverMalloc ( sizeof ( struct sockaddr_in6 ) );
    memset( addr, 0 , sizeof( struct sockaddr_in6 ) );
    int s = socket ( PF_INET6, SOCK_STREAM, 0 );
#else
    struct sockaddr_in *addr = ( struct sockaddr_in * ) WebserverMalloc ( sizeof ( struct sockaddr_in ) );
    memset( addr, 0 , sizeof( struct sockaddr_in ) );
    int s = socket ( PF_INET, SOCK_STREAM, 0 );
#endif

    if ( s == -1 )
    {
        LOG ( CONNECTION_LOG,ERROR_LEVEL,0,"socket() failed : %m %s", " " );
        WebserverFree( addr );
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

    if ( 0 != setsockopt ( s, SOL_SOCKET, SO_REUSEADDR, ( char* ) &on, sizeof ( on ) ) ){
		LOG ( CONNECTION_LOG,ERROR_LEVEL,0,"setsockopt() failed : %m %s"," " );
        WebserverFree( addr );
        close( s );
		return -2;
	}


#ifdef WEBSERVER_USE_IPV6
    if ( bind ( s, ( struct sockaddr* ) addr, sizeof ( struct sockaddr_storage ) ) == -1 )
#else
    if ( bind ( s, ( struct sockaddr* ) addr, sizeof ( struct sockaddr_in ) ) == -1 )
#endif
    {
        LOG ( CONNECTION_LOG,ERROR_LEVEL,0,"bind() failed : %m %s"," " );
        WebserverFree(addr);
        close( s );
        return -2;
    }

    if ( listen ( s, connections ) == -1 )
    {
        LOG ( CONNECTION_LOG,ERROR_LEVEL,0,"listen() failed : %m %s", " " );
        WebserverFree(addr);
        close( s );
        return -3;
    }
    WebserverFree(addr);
    return s;
}

int		PlatformShutdown(int socket){
	return shutdown(socket, SHUT_RDWR);
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

    ret = select ( select_listen_max + 1, &read_fds, &write_fds, NULL, NULL );

    if ( ret == -1 )
    {
        LOG ( CONNECTION_LOG,ERROR_LEVEL,0,"select error %d",ret );
    }
    
    return ret;
}

#endif /* SELECT_HELPER_FUNCS */

/*
 * Accept a connection on the listener sock. The peer's address, as text,
 * port and key for the per-address connection limit, is left in the
 * listener's socket_info for handleServer() to copy.
 *
 * The address comes with accept() itself; a getpeername() afterwards was a
 * second system call per connection.
 */
int		PlatformAccept(socket_info* sock, unsigned int *port)
{
	struct sockaddr_storage clientaddr;
	socklen_t addrlen = sizeof(clientaddr);
	int ret;

	memset(&sock->peer_key, 0, sizeof(sock->peer_key));
	sock->client_ip_str[0] = '\0';
	*port = 0;
#ifdef WEBSERVER_USE_IPV6
	sock->client_v6_client = 0;
#endif

	ret = accept(sock->socket, (struct sockaddr*) &clientaddr, &addrlen);
	if (ret < 0) {
		/* errno is left as accept() set it, handleServer() checks EMFILE */
		return ret;
	}

	if (clientaddr.ss_family == AF_INET) {
		struct sockaddr_in *a = (struct sockaddr_in*) &clientaddr;
		*port = ntohs(a->sin_port);
		inet_ntop(AF_INET, &a->sin_addr, sock->client_ip_str, sizeof(sock->client_ip_str));
		connLimitKeyFromAddress(&a->sin_addr, 4, &sock->peer_key);
	} else if (clientaddr.ss_family == AF_INET6) {
		struct sockaddr_in6 *a = (struct sockaddr_in6*) &clientaddr;
		*port = ntohs(a->sin6_port);
		/* Without WEBSERVER_USE_IPV6 the text buffer is too small for it and
		 * stays empty; the server then only listens on IPv4 anyway */
		inet_ntop(AF_INET6, &a->sin6_addr, sock->client_ip_str, sizeof(sock->client_ip_str));
#ifdef WEBSERVER_USE_IPV6
		sock->client_v6_client = IN6_IS_ADDR_V4MAPPED(&a->sin6_addr) ? 0 : 1;
#endif
		connLimitKeyFromAddress(&a->sin6_addr, 16, &sock->peer_key);
	} else {
		/* Unknown address type: counted in one shared group, not unlimited */
		connLimitKeyFromAddress(NULL, 0, &sock->peer_key);
	}

	return ret;
}

int     PlatformSendSocket ( int socket, const unsigned char *buf, SIZE_TYPE len, int flags )
{
    int ret;
    int ret2;
	SIZE_TYPE send_bytes = 0;

	if ( len > INT_MAX ){
		LOG ( CONNECTION_LOG,ERROR_LEVEL,socket,"len > INT_MAX ( %"SIZE_TYPE_PRINT_DEZ" / %d ) ",len,INT_MAX );
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
                continue;
            }
            else
            {
                break;
            }
        }
        send_bytes += (SIZE_TYPE)ret;
        if ( send_bytes == len ){
            break;
        }
    }
    return ret;
}

int     PlatformSendSocketNonBlocking ( int socket, const unsigned char *buf, SIZE_TYPE len, int flags )
{
    int ret;
    int ret2;
    ret = send ( socket,buf,len,flags );
    if ( ret == -1 )
    {
        ret2 = errno;
        if ( ( ret2 == EWOULDBLOCK ) || ( ret2 == EAGAIN ) )
        {
            return CLIENT_SEND_BUFFER_FULL;
        }
        if ( errno == ECONNRESET )
        {
            LOG ( CONNECTION_LOG,ERROR_LEVEL,socket,"%s","ECONNRESET" );
            return CLIENT_DISCONNECTED;
        }
        if ( errno == EPIPE )
        {
            LOG ( CONNECTION_LOG,ERROR_LEVEL,socket,"%s","EPIPE" );
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
		LOG ( CONNECTION_LOG,ERROR_LEVEL,socket,"len > INT_MAX ( %"SIZE_TYPE_PRINT_DEZ" / %d ) ",len,INT_MAX );
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

            LOG ( CONNECTION_LOG,ERROR_LEVEL,socket,"%s","readSocket Error");
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




