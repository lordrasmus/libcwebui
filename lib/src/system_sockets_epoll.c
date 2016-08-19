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



#ifdef USE_EPOLL

#ifdef WEBSERVER_USE_SSL
	#error "SSL mit epoll nicht implementiert"
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




void eventHandler(int a, short b, void *t) {
	char pers = 0;

	printf("eventHandler  ");
	printf("Nicht implementiert\n");
	exit(1);
	
	#ifdef _WEBSERVER_MEMORY_DEBUG_
		if ( print_blocks_now == 1 ){
			printBlocks();
			print_blocks_now = 0;
		}
	#endif
	
	short tmp = b & ~( EV_TIMEOUT | EV_READ | EV_WRITE | EV_SIGNAL | EV_PERSIST | EV_ET ) ;
	
	if ( tmp != 0 ){
		//
		// Dürfte nie auftreten, es scheint das eine Event für eine bereits freigegeben Struktur aufgerufen wird
		// möglicherweise Problem mit Websockets und / oder OpenSSL
		printf("BUG: Invalid Flags 0x%X\n",tmp);
		return;
	}

	//printf("B: %d\n",b);

	if ((b & EV_PERSIST) == EV_PERSIST) {
		pers = EVENT_PERSIST;
	}
	
	if ((b & EV_SIGNAL) == EV_SIGNAL) {
		printf("SIGNAL ignored a: %d  b : %d  t: 0x%p \n",a,b,t);
		//handleer(a, EVENT_SIGNAL | pers, t);
		fflush(stdout);
		return;
	}

	if ((b & EV_TIMEOUT) == EV_TIMEOUT) {
		//printf("%d TIMEOUT \n",a);
		handleer(a, EVENT_TIMEOUT | pers, t);
		fflush(stdout);
		return;
	}

	if ((b & EV_READ) == EV_READ) {
		//printf("%d READ \n",a);
		handleer(a, EVENT_READ | pers, t);
		fflush(stdout);
		return;
	}

	if ((b & EV_WRITE) == EV_WRITE) {
		//printf("%d WRITE \n",a);
		handleer(a, EVENT_WRITE | pers, t);
		fflush(stdout);
		return;
	}

}

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
			client_sock[i].flags = EV_READ;
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
	
	printf("\naddEventSocketRead : %d\n\n",sock->socket);
	
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
			client_sock[i].flags = EV_READ | EV_PERSIST;
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
		
	printf("\naddEventSocketReadPersist : %d\n\n",sock->socket);

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
			client_sock[i].flags = EV_WRITE | EV_PERSIST;
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
		
	printf("\naddEventSocketWritePersist : %d\n\n",sock->socket);

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
	
	 //FD_CLR(sock3, &gesamt_sock);  //aus Menge löschen
     //client_sock[i] = -1;          //auf -1 setzen
     //printf("Ein Client hat sich beendet\n");
     
	printf("delEventSocketReadPersist ");
	printf("Nicht implementiert\n");
	exit(1);
	
}


void delEventSocketWrite2( int socket ) {
	int i;
	
    for( i=0; i< FD_SETSIZE; i++){
		if( (client_sock[i].socket == socket) && ( (client_sock[i].flags & EV_WRITE ) == EV_WRITE ) ) {
			client_sock[i].socket = -1;
			client_sock[i].sock = 0;
			client_sock[i].flags = 0;
			
			FD_CLR(socket, &gesamt_schreibe_sockets);
             
			printf("\ndelEventSocketWrite2 : %d\n\n",socket);
            return;
		}
	}
	
	         
	printf("\ndelEventSocketWrite2 : not found %d\n\n",socket);
}

void delEventSocketWritePersist(socket_info* sock) {
	
	 printf("\ndelEventSocketWritePersist : %d\n\n",sock->socket);
	 delEventSocketWrite2( sock->socket );
}

void delEventSocketRead2( int socket ) {
	int i;
	
    for( i=0; i< FD_SETSIZE; i++){
		if( (client_sock[i].socket == socket) && ( (client_sock[i].flags & EV_READ ) == EV_READ ) ){
			client_sock[i].socket = -1;
			client_sock[i].sock = 0;
			client_sock[i].flags = 0;
			
			FD_CLR(socket, &gesamt_lese_sockets);
             
			printf("\ndelEventSocketRead2 : %d\n\n",socket);
            return;
		}
	}
	
	         
	printf("\ndelEventSocketRead2 : not found %d\n\n",socket);
}

	
void delEventSocketAll2( int socket ) {
	
	int i;
	
    for( i=0; i< FD_SETSIZE; i++){
		if(client_sock[i].socket == socket) {
			client_sock[i].socket = -1;
			client_sock[i].sock = 0;
			client_sock[i].flags = 0;
			
			FD_CLR(socket, &gesamt_lese_sockets);
			FD_CLR(socket, &gesamt_schreibe_sockets);
             
			printf("\ndelEventSocketAll : %d\n\n",socket);
            return;
		}
	}
	
	         
	printf("\ndelEventSocketAll : not found %d\n\n",socket);
}

void delEventSocketAll(socket_info* sock) {
	delEventSocketAll2( sock->socket );
}

void deleteEvent(socket_info* sock){
	printf("deleteEvent ");
	printf("Nicht implementiert\n");
	exit(1);
}

void initEvents() {
	int i;
	
	for( i=0; i<FD_SETSIZE; i++){
		client_sock[i].socket = -1;
		client_sock[i].sock = 0;
	}
		
	FD_ZERO(&gesamt_lese_sockets);
	FD_ZERO(&gesamt_schreibe_sockets);
	//FD_SET(sock1, &gesamt_sock);
}

int check_sock_exists( int socket ){
	int i;
	
	for( i=0; i< FD_SETSIZE; i++){
		if(client_sock[i].socket == socket) {
			return 1;
		}
	}
	return 0;
}

char waitEvents() {
	int i;
	int sock3;
	int ready;
	fd_set lese_sock;
	fd_set send_sock;
	
	while( 1 ){
		printf("select max : %d  sock_max : %d\n", max,  sock_max);
		fflush(stdout);
		
		lese_sock = gesamt_lese_sockets;
		send_sock = gesamt_schreibe_sockets;
		
		for(i=0; i<=max; i++) {
			printf("Sock %d : %d 0x%X ( %p )\n",i,client_sock[i].socket, client_sock[i].flags, client_sock[i].sock);
			
		}
		
		ready = select( sock_max+1, &lese_sock, &send_sock, NULL, NULL ); // &send_sock
		
		printf("select ret : %d\n",ready);
		fflush(stdout);
		
		for(i=0; i<=max; i++) {
			
			if(( sock3 = client_sock[i].socket) < 0)
				continue;
			
			if(FD_ISSET(sock3, &lese_sock)){
			  
				if ( ( client_sock[i].flags & EV_PERSIST ) == EV_PERSIST ){
					printf("Socket %d Read\n",sock3);
					handleer(sock3 , EVENT_READ , client_sock[i].sock );
				}else{			  
					int socket = client_sock[i].sock->socket;
					handleer(sock3 , EVENT_READ , client_sock[i].sock );
					
					#ifdef WEBSERVER_USE_SSL
					if ( ( client_sock[i].socket == socket ) && ( client_sock[i].sock->ssl_event_flags & EV_READ ) ){
						client_sock[i].sock->ssl_event_flags &= ~EV_READ;
					}else{
					#endif
						printf("Socket %d Read not Persist\n",sock3);
						delEventSocketRead2( socket );
					#ifdef WEBSERVER_USE_SSL
					}
					#endif
				}
				
				if( --ready <= 0 )
					break;
			}

			if(FD_ISSET(sock3, &send_sock)){
				
				if ( ( client_sock[i].flags & EV_PERSIST ) == EV_PERSIST ){
					printf("Socket %d Write\n",sock3);
					handleer(sock3 , EVENT_WRITE , client_sock[i].sock );
				}else{
					int socket = client_sock[i].sock->socket;
					handleer(sock3 , EVENT_WRITE , client_sock[i].sock );
					
					#ifdef WEBSERVER_USE_SSL
					if ( ( client_sock[i].socket == socket ) &&  ( client_sock[i].sock->ssl_event_flags & EVENT_WRITE  ) ){
						client_sock[i].sock->ssl_event_flags &= ~EVENT_WRITE;
					}else{
					#endif
						printf("Socket %d Write not Persist\n",sock3);
						delEventSocketWrite2( socket );
					#ifdef WEBSERVER_USE_SSL
					}
					#endif
				}
				
				if( --ready <= 0 )
					break;
			}
			
		}
		
		//sleep(1);
	}
    
    return 0;
    
}

void breakEvents(void){
		printf("breakEvents ");
	printf("Nicht implementiert\n");
	exit(1);
}

void freeEvents() {
		printf("freeEvents");
	printf(" Nicht implementiert\n");
	exit(1);
}






#endif

