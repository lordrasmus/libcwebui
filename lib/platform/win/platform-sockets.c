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
#include <winsock2.h>
#include <strsafe.h>

#include "webserver.h"


fd_set fds;  
int select_listen_sockets;
int select_listen_max;

/**************************************************************
*                                                             *
*                   Netzwerk Operationen                      *
*                                                             *
**************************************************************/

void	PlatformInitNetwork(void){
	WSADATA wsa;
    if (WSAStartup(MAKEWORD(1, 1), &wsa))
    {
        WebServerPrintf("WSAStartup() failed, %lu\n", (unsigned long)GetLastError());
        return;
    }
}

void	PlatformEndNetwork(void){
	WSACleanup();
}

int		PlatformSetNonBlocking(int socket){
	u_long iMode = 1;
	int ret;
	ret = ioctlsocket(socket, FIONBIO, &iMode);
	return iMode;

	/*long arg; 
	// Set non-blocking 
	if( (arg = fcntl(socket, F_GETFL, NULL)) < 0) { 
		fprintf(stderr, "Error fcntl(..., F_GETFL) (%s)\n", strerror(errno)); 
		exit(0); 
	} 
	arg |= O_NONBLOCK; 
	if( fcntl(socket, F_SETFL, arg) < 0) { 
		fprintf(stderr, "Error fcntl(..., F_SETFL) (%s)\n", strerror(errno)); 
		exit(0); 
	} 
	return 1;*/
}

int		PlatformSetBlocking(int socket){
	u_long iMode = 0;
	ioctlsocket(socket, FIONBIO, &iMode);
	return iMode;
}

int		PlatformGetSocket(unsigned short port,int connections)
{	
	int on;
	struct sockaddr_in *addr = malloc(sizeof(struct sockaddr_in));
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

int		PlatformClose(int socket) {
	return _close(socket);
}
int		PlatformShutdown(int socket) {
	return shutdown(socket, SD_BOTH);
}

void	PlatformInitSelect(void){
	FD_ZERO(&fds); 
	select_listen_sockets = 0;
	select_listen_max = 0;
}

void	PlatformAddSelectSocket(char write,int socket){
	FD_SET(socket, &fds);
	select_listen_sockets++;
	if (socket > select_listen_max)
		select_listen_max = socket;
	#pragma message ( "neu schreiben" )

}

int		PlatformSelectGetActiveSocket(char write,int socket){
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

int		PlatformAccept(socket_info* sock, unsigned int *port)
{
	struct sockaddr_in addr;
	int l = sizeof(struct sockaddr_in);
	int ret;
	
	ret = accept(sock->socket, (struct sockaddr*)&addr,&l);
	//snprintf(buffer,256,"%s",inet_ntoa(addr.sin_addr));	
	*port = addr.sin_port;
	
	return ret;		
}

void PlatformGetPeerName(socket_info* sock){
	struct sockaddr_in addr;
	socklen_t len;

	len = sizeof(addr);
	getpeername(sock->socket, (struct sockaddr*)&addr, &len);
	//sprintf(sock->client_ip_str,"%s",inet_ntoa(addr.sin_addr));
	//printf("Peer IP address: %s\n", inet_ntoa(m_addr.sin_addr));
}

int     PlatformSendSocket(int socket, const unsigned char *buf, SIZE_TYPE len, int flags)
{
	//return send(socket,buf,len,flags);
	int ret,ret2;
	
	while(1){
		ret = send(socket,buf,len,flags);
		if(ret == -1){
			ret2 = WSAGetLastError();
			if( ( ret2 == WSAEWOULDBLOCK ) ){ // || ( ret2 == EAGAIN ) ){
				return CLIENT_SEND_BUFFER_FULL;				
			}else{
				break;
			}
		}
		if(ret == len )
			break;
	}
	return ret;
}

int     PlatformSendSocketNonBlocking(int socket, const unsigned char *buf, SIZE_TYPE len, int flags)
{
	//return send(socket,buf,len,flags);
	int ret,ret2;
	ret = send(socket,buf,len,flags);
	
	if(ret == -1){
		ret2 = WSAGetLastError();
		if( ( ret2 == WSAEWOULDBLOCK ) ) {// || ( ret2 == EAGAIN ) ){
			return CLIENT_SEND_BUFFER_FULL;				
		}

		if ( ret2 == WSAECONNRESET  ){
			printf("WSAECONNRESET : An existing connection was forcibly closed by the remote host.\n");
			return CLIENT_DISCONNECTED;
		}
				
		//perror("PlatformSendSocketNonBlocking");
		printf("PlatformSendSocketNonBlocking Unhandled Error %d\n",ret2);
	
		return CLIENT_DISCONNECTED;
	}
	
	return ret;
}

int     PlatformRecvSocket(int socket, unsigned char *buf, SIZE_TYPE len, int flags)
{
	return recv(socket,buf,len,flags);
}

int     PlatformRecvSocketNonBlocking(int socket, unsigned char *buf, SIZE_TYPE len, int flags)
{
	int len2 = 0;
	int ret;

	
	ret = recv(socket,buf,len,flags);
	
	if(ret == 0){
		return CLIENT_DISCONNECTED;		// TODO: muss das rein oder nicht ?
	}

	if(ret == SOCKET_ERROR ){			
		int nError=WSAGetLastError();
		if( nError == WSAEWOULDBLOCK ){
			return CLIENT_NO_MORE_DATA;							
		}

		if( nError == WSAECONNABORTED ){
			return CLIENT_DISCONNECTED;							
		}
			
		//perror("readSocket Error");				 
		printf("Error Code : %d\n",nError);			
		return CLIENT_DISCONNECTED;
	}
		
	return ret;	
}
	

int     PlatformCloseSocket(int socket)
{
	return closesocket(socket);
}




