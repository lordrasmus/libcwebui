/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/


#include <stdio.h>


#include <errno.h>

//#include "bsp.h"
#include "sockapi.h"
#include "tx_api.h"

#include "system.h"
#include "../WebserverConfig.h"


fd_set fds;  
int select_listen_sockets;
int select_listen_max;

/**************************************************************
*                                                             *
*                   Netzwerk Operationen                      *
*                                                             *
**************************************************************/

void	PlatformInitNetwork(void){}

void	PlatformEndNetwork(void){}

int		PlatformSetNonBlocking(int socket){
	long arg; 
	char optval[100];
	int optlen;
	int ret;
	// Set non-blocking 
	/*if( (arg = fcntl(socket, F_GETFL, NULL)) < 0) { 
		fprintf(stderr, "Error fcntl(..., F_GETFL) (%s)\n", strerror(errno)); 
		exit(0); 
	} 
	arg |= O_NONBLOCK; 
	if( fcntl(socket, F_SETFL, arg) < 0) { 
		fprintf(stderr, "Error fcntl(..., F_SETFL) (%s)\n", strerror(errno)); 
		exit(0); 
	} */
	
	ret = getsockopt(socket, SOL_SOCKET, SO_NONBLOCK, (char*)&arg, &optlen);
	arg = 1;
	ret = setsockopt(socket, SOL_SOCKET, SO_NONBLOCK, (char*)&arg, optlen);
	
	ret = getsockopt(socket, SOL_SOCKET, SO_NONBLOCK, (char*)&arg, &optlen);
	
	
	return 1;
}

int		PlatformSetBlocking(int socket){
	long arg; 
	int optlen;
	int ret;
	// Set blocking 
	/*if( (arg = fcntl(socket, F_GETFL, NULL)) < 0) { 
		fprintf(stderr, "Error fcntl(..., F_GETFL) (%s)\n", strerror(errno)); 
		exit(0); 
	} 
	arg &= ~O_NONBLOCK; 
	if( fcntl(socket, F_SETFL, arg) < 0) { 
		fprintf(stderr, "Error fcntl(..., F_SETFL) (%s)\n", strerror(errno)); 
		exit(0); 
	} */
	
	ret = getsockopt(socket, SOL_SOCKET, SO_NONBLOCK, (char*)&arg, &optlen);
	arg = 1;
	ret = setsockopt(socket, SOL_SOCKET, SO_NONBLOCK, (char*)&arg, optlen);
	
	ret = getsockopt(socket, SOL_SOCKET, SO_NONBLOCK, (char*)&arg, &optlen);
	return 1;
}

int		PlatformGetSocket( int port,int connections)
{	
	int on;
	struct sockaddr_in *addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
	int s = socket(PF_INET, SOCK_STREAM, 0);
    if (s == -1)
    {
        perror("socket() failed");
        return -1;
    }

    addr->sin_addr.s_addr = INADDR_ANY;
    addr->sin_port = htons(port);
    addr->sin_family = AF_INET;

	setsockopt( s, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on) );

    if (bind(s, (struct sockaddr*)addr, sizeof(struct sockaddr_in)) == -1)
    {
        perror("bind() failed");
        return -2;
    }

    if (listen(s, connections) == -1)
    {
        perror("listen() failed");
        return -3;
    }

	
	return s;
}



void	PlatformInitSelect(void){
	FD_ZERO(&fds); 
	select_listen_sockets = 0;
	select_listen_max = 0;
}

void	PlatformAddSelectSocket(int socket){
	FD_SET(socket, &fds);
	select_listen_sockets++;
	if (socket > select_listen_max)
		select_listen_max = socket;
}

int		PlatformSelectGetActiveSocket(int socket){
	return FD_ISSET(socket,&fds);
}

int		PlatformSelect(void){
	int ret;
	
	//printf("Warte auf select .. ");
	//fflush(stdout);
	
	ret = select(select_listen_max + 1, &fds, NULL, NULL, NULL);
	
	if(ret == -1){
		printf("select error %d\n",ret);
	}else{
		//printf("OK\n");
	}
	
	return ret;
}

int		PlatformAccept(int socket){
	return accept(socket, NULL, 0);
}

int     PlatformSendSocket(int socket, unsigned char *buf, int len, int flags)
{
	int ret,ret2;
	
	while(1){
		ret = send(socket,buf,len,flags);
		if(ret == -1){
			ret2 = getErrno();
			if( ( ret2 == EWOULDBLOCK ) || ( ret2 == EAGAIN ) ){
				//tx_thread_sleep(100);
				continue;
			}else{
				break;
			}
		}
		if(ret == len )
			break;
	}
	return ret;
}

int     PlatformRecvSocket(int socket, unsigned char *buf, int len, int flags)
{
	return recv(socket,buf,len,flags);
}

int     PlatformRecvSocketNonBlocking(int socket, unsigned char *buf, int len, int flags)
{
	int len2 = 0;
	int ret;
	int err_ret;

	while(1){
		ret = recv(socket,buf,len-len2,flags);
		if(ret == 0){
			/*if(len2 == 0){
				return CLIENT_DISCONNECTED;
			}
			return len2;*/
			return CLIENT_DISCONNECTED;
		}
		if(ret < 0){
			err_ret = getErrno() ;
			if ( (  err_ret  == EAGAIN) || (  err_ret  == EWOULDBLOCK) ){
				printf("No Data Aviable\n");
				if( len2 == 0){
					return CLIENT_NO_MORE_DATA;
				}else{
					return len2;
				}
			}
				
			perror("readSocket Error");				 
			printf("bytes : %d\n",ret);			
			return CLIENT_UNKNOWN_ERROR;
		}
		
		len2 +=ret;
		if(len == len2)
			return len2;
	}
	
	
	return CLIENT_UNKNOWN_ERROR;
}

int     PlatformCloseSocket(int socket)
{
	return close(socket);
}




