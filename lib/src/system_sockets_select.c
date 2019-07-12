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


#ifdef DMALLOC
#include <dmalloc/dmalloc.h>
#endif


#ifdef USE_SELECT

/* #define DEBUG_SELECT */

#ifdef WEBSERVER_USE_SSL
	#error "SSL mit select nicht implementiert"
#endif

struct client_socket{
	int socket;
	uint16_t flags;
	socket_info* sock;
};

static struct client_socket client_sock[FD_SETSIZE];
static fd_set gesamt_lese_sockets;
static fd_set gesamt_schreibe_sockets;
static int sock_max = 0,max = 0;



void addEventSocketRead(socket_info* sock) {
	int i;
	
	#ifdef WEBSERVER_USE_SSL
	if(sock->ssl_block_event_flags == 1){
		sock->ssl_event_flags = EV_READ;
		return;
	}
	#endif
	
	for( i=0; i< FD_SETSIZE; i++){
		if(client_sock[i].socket < 0) {
			client_sock[i].socket = sock->socket;
			client_sock[i].sock = sock;
			client_sock[i].flags = EVENT_READ;
            break;
		}
	}
          
	if( i == FD_SETSIZE ){
		printf("Maximale Anzahl an Sockets erreicht");
		exit(1);
	}
	
	if( sock->socket > sock_max )
		sock_max = sock->socket;
          
    if( i > max )
		max = i;
		
	FD_SET(sock->socket, &gesamt_lese_sockets);
	
	#ifdef DEBUG_SELECT
		printf("\naddEventSocketRead : %d\n\n",sock->socket);
	#endif
	
}

void addEventSocketReadPersist(socket_info* sock) {
	int i;
	
	#ifdef WEBSERVER_USE_SSL
	if(sock->ssl_block_event_flags == 1){
		sock->ssl_event_flags = EV_READ | EV_PERSIST;
		return;
	}
	#endif
	
    for( i=0; i< FD_SETSIZE; i++){
		if(client_sock[i].socket < 0) {
			client_sock[i].socket = sock->socket;
			client_sock[i].sock = sock;
			client_sock[i].flags = EVENT_READ | EVENT_PERSIST;
            break;
		}
	}
          
	if( i == FD_SETSIZE ){
		printf("Maximale Anzahl an Sockets erreicht");
		exit(1);
	}
	
	if( sock->socket > sock_max )
		sock_max = sock->socket;
          
    if( i > max )
		max = i;
		
	FD_SET(sock->socket, &gesamt_lese_sockets);

	#ifdef DEBUG_SELECT
		printf("\naddEventSocketReadPersist : %d\n\n",sock->socket);
	#endif

}

void addEventSocketWrite(socket_info* sock) {

	printf("addEventSocketWrite ");
	printf("Nicht implementiert\n");
	exit(1);
}

void addEventSocketWritePersist(socket_info* sock) {

	int i;
	
	#ifdef WEBSERVER_USE_SSL
	if(sock->ssl_block_event_flags == 1){
		sock->ssl_event_flags = EV_WRITE | EV_PERSIST;
		return;
	}
	#endif
	
    for( i=0; i< FD_SETSIZE; i++){
		if(client_sock[i].socket < 0) {
			client_sock[i].socket = sock->socket;
			client_sock[i].sock = sock;
			client_sock[i].flags = EVENT_WRITE | EVENT_PERSIST;
            break;
		}
	}
          
	if( i == FD_SETSIZE ){
		printf("Maximale Anzahl an Sockets erreicht");
		exit(1);
	}
	
	if( sock->socket > sock_max )
		sock_max = sock->socket;
          
    if( i > max )
		max = i;
		
	FD_SET(sock->socket, &gesamt_schreibe_sockets);

	#ifdef DEBUG_SELECT
		printf("\naddEventSocketWritePersist : %d\n\n",sock->socket);
	#endif

}

void addEventSocketReadWritePersist(socket_info* sock) {
	
	printf("addEventSocketReadWritePersist ");
	printf("Nicht implementiert\n");
	exit(1);

}

#ifdef WEBSERVER_USE_SSL
	
void commitSslEventFlags( socket_info* sock ) {
	

	
	sock->ssl_block_event_flags = 0;
	
	printf("commitSslEventFlags 0x%X  %d\n", sock->ssl_event_flags, sock->socket );
	
	
	
	/*if ( sock->ssl_event_flags & EV_WRITE ){
		if ( sock->ssl_event_flags & EV_PERSIST ) {
			addEventSocketWritePersist( sock );
		}else{
			addEventSocketWrite( sock );
		}
	}
	
	if ( sock->ssl_event_flags & EV_READ ){
		if ( sock->ssl_event_flags & EV_PERSIST ) {
			addEventSocketReadPersist( sock );
		}else{
			addEventSocketRead( sock );
		}
	}*/
	
	
}

#endif


void delEventSocketReadPersist(socket_info* sock) {
	
     
	printf("delEventSocketReadPersist ");
	printf("Nicht implementiert\n");
	exit(1);
	
}


static void delEventSocketWrite2( int socket ) {
	int i;
	
    for( i=0; i< FD_SETSIZE; i++){
		if( (client_sock[i].socket == socket) && ( (client_sock[i].flags & EVENT_WRITE ) == EVENT_WRITE ) ) {
			client_sock[i].socket = -1;
			client_sock[i].sock = 0;
			client_sock[i].flags = 0;
			
			FD_CLR(socket, &gesamt_schreibe_sockets);

            #ifdef DEBUG_SELECT
				printf("\ndelEventSocketWrite2 : %d\n\n",socket);
			#endif
            return;
		}
	}


	/* printf("\ndelEventSocketWrite2 : not found %d\n\n",socket); */
}

void delEventSocketWritePersist(socket_info* sock) {

	 /* printf("\ndelEventSocketWritePersist : %d\n\n",sock->socket); */

	 delEventSocketWrite2( sock->socket );
}

static void delEventSocketRead2( int socket ) {
	int i;
	
    for( i=0; i< FD_SETSIZE; i++){
		if( (client_sock[i].socket == socket) && ( (client_sock[i].flags & EVENT_READ ) == EVENT_READ ) ){
			client_sock[i].socket = -1;
			client_sock[i].sock = 0;
			client_sock[i].flags = 0;
			
			FD_CLR(socket, &gesamt_lese_sockets);

            #ifdef DEBUG_SELECT
				printf("\ndelEventSocketRead2 : %d\n\n",socket);
			#endif
            return;
		}
	}
	
	         
	/* printf("\ndelEventSocketRead2 : not found %d\n\n",socket); */
}

	
static void delEventSocketAll2( int socket ) {
	
	int i;
	
    for( i=0; i< FD_SETSIZE; i++){
		if(client_sock[i].socket == socket) {
			client_sock[i].socket = -1;
			client_sock[i].sock = 0;
			client_sock[i].flags = 0;
			
			FD_CLR(socket, &gesamt_lese_sockets);
			FD_CLR(socket, &gesamt_schreibe_sockets);

            #ifdef DEBUG_SELECT
				printf("\ndelEventSocketAll : %d\n\n",socket);
			#endif
            return;
		}
	}
	
	         
	/* printf("\ndelEventSocketAll : not found %d\n\n",socket); */
}

void delEventSocketAll(socket_info* sock) {
	delEventSocketAll2( sock->socket );
}

void deleteEvent(socket_info* sock){
	printf("deleteEvent ");
	printf("Nicht implementiert\n");
	exit(1);
}

void initEvents( void ) {
	int i;
	
	for( i=0; i<FD_SETSIZE; i++){
		client_sock[i].socket = -1;
		client_sock[i].sock = 0;
	}
		
	FD_ZERO(&gesamt_lese_sockets);
	FD_ZERO(&gesamt_schreibe_sockets);
}

#if 0
static int check_sock_exists( int socket ){
	int i;
	
	for( i=0; i< FD_SETSIZE; i++){
		if(client_sock[i].socket == socket) {
			return 1;
		}
	}
	return 0;
}
#endif

char waitEvents( void ) {
	int i;
	int sock3;
	int ready;
	fd_set lese_sock;
	fd_set send_sock;
	
	while( 1 ){
		#ifdef DEBUG_SELECT
			printf("select max : %d  sock_max : %d\n", max,  sock_max);
			fflush(stdout);
		#endif
		
		lese_sock = gesamt_lese_sockets;
		send_sock = gesamt_schreibe_sockets;
		
		#ifdef DEBUG_SELECT
			for(i=0; i<=max; i++) {
				if ( client_sock[i].socket > -1 ){
					printf("Sock %d : %d 0x%X ( %p )\n",i,client_sock[i].socket, client_sock[i].flags, client_sock[i].sock);
				}
			}
		#endif
		
		ready = select( sock_max+1, &lese_sock, &send_sock, NULL, NULL );
		
		#ifdef DEBUG_SELECT
			printf("select ret : %d\n",ready);
			fflush(stdout);
		#endif
		
		for(i=0; i<=max; i++) {
			
			if(( sock3 = client_sock[i].socket) < 0)
				continue;

			if(FD_ISSET(sock3, &lese_sock)){
				int totalPending; 
				socket_info *client_sock_info = client_sock[i].sock;

				if ( ( client_sock[i].flags & EVENT_PERSIST ) == EVENT_PERSIST ){
					#ifdef DEBUG_SELECT
						printf("Socket %d ( %d ) Read\n",sock3,i);
						fflush(stdout);
					#endif
					handleer(sock3 , EVENT_READ , client_sock[i].sock );
				}else{
					int socket = client_sock[i].sock->socket;
					
					#ifdef DEBUG_SELECT
						printf("Socket %d Read not Persist\n",sock3);
						fflush(stdout);
					#endif

					delEventSocketRead2( socket );

					handleer(sock3 , EVENT_READ , client_sock_info );

				}
#ifdef __ZEPHYR__
	#warning how to get pending bytes on the sockets ??
#else
	#warning ist das wirklich nötig mit dem pending ??
#ifdef _MSC_VER
				if (ioctlsocket(sock3, FIONREAD, &totalPending) == -1)
#else
				if( ioctl( sock3, FIONREAD, &totalPending) == -1 )
#endif


				{
					if ( totalPending > 0 ){
						/*#ifdef DEBUG_SELECT */
							printf("\nPending ( %d ): %d\n\n", sock3, totalPending);
							fflush(stdout);
						/*#endif */

						handleer(sock3 , EVENT_READ , client_sock_info );
					}
				}
#endif
				if( --ready <= 0 )
					break;
			}

			if(FD_ISSET(sock3, &send_sock)){
				
				if ( ( client_sock[i].flags & EVENT_PERSIST ) == EVENT_PERSIST ){
					#ifdef DEBUG_SELECT
						printf("Socket %d Write\n",sock3);
						fflush(stdout);
					#endif
					handleer(sock3 , EVENT_WRITE , client_sock[i].sock );
				}else{
					int socket = client_sock[i].sock->socket;
					
					delEventSocketWrite2( socket );

					#ifdef DEBUG_SELECT
						printf("Socket %d Write not Persist\n",sock3);
						fflush(stdout);
					#endif

					handleer(sock3 , EVENT_WRITE , client_sock[i].sock );

				}

				if( --ready <= 0 )
					break;
			}
			
		}

	}
    
    return 0;
    
}

void breakEvents(void){
		printf("breakEvents ");
	printf("Nicht implementiert\n");
	exit(1);
}

void freeEvents( void ) {
		printf("freeEvents");
	printf(" Nicht implementiert\n");
	exit(1);
}






#endif

