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

/* Schuetzt client_sock[], die globalen fd_sets sowie sock_max/max gegen
 * gleichzeitigen Zugriff aus dem Event-Loop-Thread (waitEvents) und den
 * Websocket-Threads (activateEventSocketWrite / updateWebsocketEventFlags).
 *
 * WICHTIG - Lock-Hierarchie: socket_mutex (aussen) > select_lock (innen).
 * select_lock ist ein Blatt-Lock und wird NIEMALS ueber einen handleer()-
 * Aufruf hinweg gehalten (handleer nimmt socket_mutex). Andernfalls entstuende
 * derselbe ABBA-Deadlock, den das fruehere libevent-Backend hatte. Die public
 * API nimmt select_lock am Eingang und ruft die lock-freien internen Helfer
 * (delEventSocket{Read,Write,All}2); waitEvents haelt den Lock selbst und ruft
 * ebenfalls nur diese internen Helfer. */
static WS_MUTEX select_lock;



static __thread int main_thread = 0;

void _set_main_thread( void ){
	main_thread = 1;
}

static int select_signal_pipe[2] = { 0 , 0 };

static volatile int break_loop = 0;

static void _check_select_pipe( void ){
	if ( select_signal_pipe[0] == 0 ){
		pipe( select_signal_pipe );
	}
}

static void _signal_pipe( void ){

	_check_select_pipe();

	if ( main_thread == 0 ){
		char t = 1;
		write( select_signal_pipe[1] , &t , 1 );
	}
}


static void _read_select_pipe( void ){
	char t;
	read( select_signal_pipe[0] , &t , 1 );
}




/* Lock-freier interner Helfer: traegt sock mit den gegebenen flags in die
 * client_sock-Tabelle ein. Der Aufrufer MUSS select_lock halten. */
static void register_socket( socket_info* sock, uint16_t flags ) {
	int i;

	for( i=0; i< FD_SETSIZE; i++){
		if(client_sock[i].socket < 0) {
			client_sock[i].socket = sock->socket;
			client_sock[i].sock = sock;
			client_sock[i].flags = flags;
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

	if ( flags & EVENT_READ )
		FD_SET(sock->socket, &gesamt_lese_sockets);
	if ( flags & EVENT_WRITE )
		FD_SET(sock->socket, &gesamt_schreibe_sockets);
}


void addEventSocketRead(socket_info* sock) {
	PlatformLockMutex(&select_lock);
	register_socket( sock, EVENT_READ );
	PlatformUnlockMutex(&select_lock);

	#ifdef DEBUG_SELECT
		printf("\naddEventSocketRead : %d\n\n",sock->socket);
	#endif
}

void addEventSocketReadPersist(socket_info* sock) {
	PlatformLockMutex(&select_lock);
	register_socket( sock, EVENT_READ | EVENT_PERSIST );
	PlatformUnlockMutex(&select_lock);

	#ifdef DEBUG_SELECT
		printf("\naddEventSocketReadPersist : %d\n\n",sock->socket);
	#endif
}

void addEventSocketWritePersist(socket_info* sock) {
	PlatformLockMutex(&select_lock);
	register_socket( sock, EVENT_WRITE | EVENT_PERSIST );
	PlatformUnlockMutex(&select_lock);

	#ifdef DEBUG_SELECT
		printf("\naddEventSocketWritePersist : %d\n\n",sock->socket);
	#endif
}

void addEventSocketReadWritePersist(socket_info* sock) {
	PlatformLockMutex(&select_lock);
	register_socket( sock, EVENT_WRITE | EVENT_READ | EVENT_PERSIST );
	PlatformUnlockMutex(&select_lock);

	_signal_pipe();

	#ifdef DEBUG_SELECT
		printf("\naddEventSocketReadWritePersist : %d\n\n", sock->socket);
	#endif
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
	PlatformLockMutex(&select_lock);
	delEventSocketWrite2( sock->socket );
	PlatformUnlockMutex(&select_lock);
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

void delEventSocketReadPersist(socket_info* sock) {
	PlatformLockMutex(&select_lock);
	delEventSocketAll2( sock->socket );
	PlatformUnlockMutex(&select_lock);
}

void delEventSocketAll(socket_info* sock) {
	PlatformLockMutex(&select_lock);
	delEventSocketAll2( sock->socket );
	PlatformUnlockMutex(&select_lock);
}

static void updateEventSocketWrite( socket_info* sock, int enable ) {
	int i;

	PlatformLockMutex(&select_lock);
	for( i=0; i< FD_SETSIZE; i++){
		if( client_sock[i].socket == sock->socket ) {
			if ( enable ) {
				client_sock[i].flags |= EVENT_WRITE;
				FD_SET( sock->socket, &gesamt_schreibe_sockets );
			} else {
				client_sock[i].flags &= ~EVENT_WRITE;
				FD_CLR( sock->socket, &gesamt_schreibe_sockets );
			}
			break;
		}
	}
	PlatformUnlockMutex(&select_lock);

	_signal_pipe();
}

void activateEventSocketWrite(socket_info* sock) {
	updateEventSocketWrite( sock, 1 );
}

void updateWebsocketEventFlags(socket_info* sock, int list_empty) {
	updateEventSocketWrite( sock, list_empty ? 0 : 1 );
}

void deleteEvent(socket_info* sock){
	PlatformLockMutex(&select_lock);
	delEventSocketAll2( sock->socket );
	PlatformUnlockMutex(&select_lock);
}

void initEvents( void ) {
	int i;

	PlatformCreateMutex(&select_lock);

	for( i=0; i<FD_SETSIZE; i++){
		client_sock[i].socket = -1;
		client_sock[i].sock = 0;
	}

	FD_ZERO(&gesamt_lese_sockets);
	FD_ZERO(&gesamt_schreibe_sockets);

	LOG(MESSAGE_LOG, NOTICE_LEVEL, 0, "%s", "event dispatcher: select");
}



char waitEvents( void ) {
	int i;
	int sock3;
	int ready;
	int local_sock_max;
	int local_max;
	fd_set lese_sock;
	fd_set send_sock;

	_check_select_pipe();

	while( !break_loop ){

		while(1){

			/* fd_sets und die Grenzen konsistent unter Lock schnappschiessen,
			 * damit select() und die Dispatch-Schleife denselben Stand sehen. */
			PlatformLockMutex(&select_lock);
			lese_sock = gesamt_lese_sockets;
			send_sock = gesamt_schreibe_sockets;
			local_sock_max = sock_max;
			local_max = max;
			PlatformUnlockMutex(&select_lock);

			FD_SET( select_signal_pipe[0], &lese_sock );

			ready = select( local_sock_max+1, &lese_sock, &send_sock, NULL, NULL );
			if ( ready != EINTR )
				break;
		}


		#ifdef DEBUG_SELECT
			printf("select ret : %d\n",ready);
			fflush(stdout);
		#endif

		for(i=0; i<=local_max; i++) {

			socket_info* read_sock  = 0;
			socket_info* write_sock = 0;

			/* Auswahl + ggf. Deregistrierung non-persistenter Events unter Lock,
			 * dann Lock freigeben und erst danach handleer() aufrufen (das
			 * socket_mutex nimmt) -> select_lock bleibt ein Blatt-Lock. */
			PlatformLockMutex(&select_lock);

			if(( sock3 = client_sock[i].socket) < 0){
				PlatformUnlockMutex(&select_lock);
				continue;
			}

			if(FD_ISSET(sock3, &lese_sock)){
				read_sock = client_sock[i].sock;
				if ( ( client_sock[i].flags & EVENT_PERSIST ) != EVENT_PERSIST ){
					delEventSocketRead2( sock3 );
				}
			}

			/* client_sock[i] kann durch delEventSocketRead2() oben geleert
			 * worden sein -> erneut pruefen, bevor write behandelt wird. */
			if( client_sock[i].socket == sock3 && FD_ISSET(sock3, &send_sock)){
				write_sock = client_sock[i].sock;
				if ( ( client_sock[i].flags & EVENT_PERSIST ) != EVENT_PERSIST ){
					delEventSocketWrite2( sock3 );
				}
			}

			PlatformUnlockMutex(&select_lock);

			if ( read_sock != 0 ){
				#ifdef DEBUG_SELECT
					printf("Socket %d Read\n",sock3);
					fflush(stdout);
				#endif
				handleer(sock3 , EVENT_READ , read_sock );
			}

			if ( write_sock != 0 ){
				#ifdef DEBUG_SELECT
					printf("Socket %d Write\n",sock3);
					fflush(stdout);
				#endif
				handleer(sock3 , EVENT_WRITE , write_sock );
			}

		}

		if(FD_ISSET(select_signal_pipe[0], &lese_sock)){
			_read_select_pipe();
		}
	}

	break_loop = 0;
    return 0;

}

void breakEvents(void){
	break_loop = 1;

	/* select() in waitEvents() blockiert ohne Timeout -> ueber die Signal-Pipe
	 * aufwecken, damit die Schleife das gesetzte break_loop sieht. Direkter
	 * Write (nicht _signal_pipe), da breakEvents auch aus dem Haupt-Thread
	 * aufgerufen werden kann. */
	_check_select_pipe();
	if ( select_signal_pipe[1] != 0 ){
		char t = 1;
		write( select_signal_pipe[1] , &t , 1 );
	}
}

void freeEvents( void ) {
	if ( select_signal_pipe[0] != 0 ){
		close( select_signal_pipe[0] );
		close( select_signal_pipe[1] );
		select_signal_pipe[0] = 0;
		select_signal_pipe[1] = 0;
	}
}






#endif

