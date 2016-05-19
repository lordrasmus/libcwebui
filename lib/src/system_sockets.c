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


#include <strings.h>

#include "webserver.h"

#ifdef LINUX
#include <sys/sendfile.h>
#endif

#ifdef DMALLOC
#include <dmalloc/dmalloc.h>
#endif


#ifndef WEBSERVER_MAX_POST_CONTENT_LENGTH
	#error WEBSERVER_MAX_POST_CONTENT_LENGTH muss definiert werden
#endif

/*
 http://tangentsoft.net/wskfaq/articles/bsd-compatibility.html
 http://publib.boulder.ibm.com/infocenter/iseries/v5r3/index.jsp?topic=/rzab6/rzab6xnonblock.htm
 http://www.wangafu.net/~nickm/libevent-book/01_intro.html
*/

#ifdef WEBSERVER_USE_WNFS
	/* socket für den nfs zugriff */
	int wnfs_socket = 0;
#endif

CLIENT_WRITE_DATA_STATUS handleClientWriteData(socket_info* sock);
CLIENT_WRITE_DATA_STATUS handleClientWriteDataNotCachedReadWrite(socket_info* sock);

int WebserverStartConnectionManager(void) {
	int socket;
	socket_info* info;


	socket = PlatformGetSocket(getConfigInt("port"), globals.config.connections);

	if (socket < 0) {
		LOG(CONNECTION_LOG, ERROR_LEVEL, 0, "Error : Server Socket %d konnte nicht erzeugt werden werden\n", getConfigInt("port"));
		return -1;
	}

	info = WebserverMallocSocketInfo();
	PlatformCreateMutex(&info->socket_mutex);
	info->active = 1;
	info->use_ssl = 0;
	info->port = getConfigInt("port");
	info->socket = socket;
	info->server = 1;
	PlatformSetNonBlocking(socket);
	addSocketContainer(info);
	addEventSocketReadPersist(info);



	/* AB hier wird der SSL Socket initialisiert */

#ifdef WEBSERVER_USE_SSL
	int ssl_port = getConfigInt("ssl_port");
	if ( ssl_port > 0 ){
		socket = PlatformGetSocket(getConfigInt("ssl_port"), globals.config.connections);
		if (socket < 0) {
			LOG(CONNECTION_LOG, ERROR_LEVEL, 0, "Error : Server SSL Socket %d konnte nicht erzeugt werden werden", getConfigInt("ssl_port"));
			return -1;
		}

		info = WebserverMallocSocketInfo();
		PlatformCreateMutex(&info->socket_mutex);
		info->active = 1;
		info->use_ssl = 1;
		info->port = ssl_port;
		info->socket = socket;
		info->server = 1;
		PlatformSetNonBlocking(socket);
		addSocketContainer(info);
		addEventSocketReadPersist(info);
	}

#endif

#ifdef WEBSERVER_USE_SSL
	if ( ssl_port > 0 ){
		LOG(CONNECTION_LOG, NOTICE_LEVEL, 0, "webserver listening on port %d & port %d (SSL) ( PID %d )",
			getConfigInt("port"), getConfigInt("ssl_port"), getpid());
	}else{
		LOG( CONNECTION_LOG, NOTICE_LEVEL, 0, "webserver listening on port %d ( PID %d )",
			getConfigInt("port"), getpid());
	}
#else
	LOG( CONNECTION_LOG, NOTICE_LEVEL, 0, "webserver listening on port %d ( PID %d )",
			getConfigText( "server_ip"), getConfigInt("port"), getpid());
#endif

#ifdef WEBSERVER_USE_WNFS

	socket = PlatformGetSocket(4444, 1 );
	if (socket < 0) {
		LOG(CONNECTION_LOG, ERROR_LEVEL, 0, "Error : Server SSL Socket %d konnte nicht erzeugt werden werden", getConfigInt("ssl_port"));
		return -1;
	}

	info = WebserverMallocSocketInfo();
	PlatformCreateMutex(&info->socket_mutex);
	info->active = 1;
	info->use_ssl = 0;
	info->port = 4444;
	info->socket = socket;
	info->server = 2;
	PlatformSetNonBlocking(socket);
	addSocketContainer(info);
	addEventSocketReadPersist(info);

	LOG( CONNECTION_LOG, NOTICE_LEVEL, 0, "Webserver NFS Online auf Port %d ",info->port);

#endif

	return 0;
}


socket_info* addClientSocket(int s, char ssl) {
	socket_info* info;
	info = WebserverMallocSocketInfo();
	if (info != 0) {
		PlatformCreateMutex(&info->socket_mutex);
		info->header = WebserverMallocHttpRequestHeader();
		info->use_ssl = ssl;
		info->client = 1;
		info->server = 0;
		info->active = 1;
		info->socket = s;
		addSocketContainer(info);
		addEventSocketRead(info); /*  | EV_PERSIST| EV_WRITE */
		return info;
	}
	return 0;
}

void reCopyHeaderBuffer(socket_info* sock, unsigned int end) {
	unsigned int i, i2;
	i2 = end; /* end ist das letzte geparste zeichen */
	for (i = 0; i < sock->header_buffer_pos - end; i++, i2++) {
		sock->header_buffer[i] = sock->header_buffer[i2];
	}
	sock->header_buffer[i] = '\0';
	sock->header_buffer_pos = i;
}

void WebserverConnectionManagerCloseRequest(socket_info* sock) {

	sock->active = 0;
	sock->closeSocket = 1;
#ifdef WEBSERVER_USE_WEBSOCKETS
	if ( sock->isWebsocket == 1 ) {
		/* laenge 1 um keinen malloc mit 0 bytes zu machen */
		/*
		msg = create_websocket_input_queue_msg(WEBSOCKET_SIGNAL_DISCONNECT,sock->websocket_guid,sock->header->url,1);
		insert_websocket_input_queue( msg);
		*/
#if _WEBSERVER_CONNECTION_DEBUG_ > 1
		LOG ( CONNECTION_LOG,NOTICE_LEVEL, sock->socket, "Websocket Close Request","" );
#endif
	}
#endif

	delEventSocketAll(sock);
	deleteSocket(sock);

	WebserverCloseSocket(sock);

	WebserverFreeSocketInfo(sock);
}

post_handler post_handle_func = 0;

static void call_pre_post_handler( socket_info* sock ){

	if ( post_handle_func != 0 ){
		post_handle_func( "pre", sock->header->url );
	}

}

static void call_post_post_handler( socket_info* sock ){

	if ( post_handle_func != 0 ){
		post_handle_func( "post", sock->header->url );
	}

}

int recv_post_payload( socket_info* sock, const char* buffer, uint32_t len){

	/*
	LOG(CONNECTION_LOG,NOTICE_LEVEL,sock->socket,"POST Data %d of %d -> %d",sock->header->post_buffer_pos,sock->header->contentlenght, len);
	*/

	if ( sock->header->post_buffer == 0 ){
		sock->header->post_buffer = WebserverMalloc( sock->header->contentlenght + 1 );
		/* um Payload Buffer neim Debug ausgeben zu können */
		sock->header->post_buffer[sock->header->contentlenght] = '\0';

		call_pre_post_handler( sock );

	}

	if ( ( sock->header->post_buffer_pos + len) <= sock->header->contentlenght ){

		memcpy( &sock->header->post_buffer[sock->header->post_buffer_pos], buffer , len );
		sock->header->post_buffer_pos += len;
		sock->header_buffer_pos = 0;

	}else{

		uint32_t diff = sock->header->contentlenght - sock->header->post_buffer_pos;

		memcpy( &sock->header->post_buffer[sock->header->post_buffer_pos], buffer , diff );
		sock->header->post_buffer_pos += diff;

		if ( len > diff ){
			sock->header_buffer_pos = diff;
			reCopyHeaderBuffer(sock, diff);
			#warning "verarbeitung von weiteren header bytes noch testen"

			call_post_post_handler( sock );

			return 1;
		}

		sock->header_buffer_pos = 0;
	}

	if ( sock->header->post_buffer_pos == sock->header->contentlenght ){

		call_post_post_handler( sock );

		return 1;
	}

	return 0;
}


static int check_post_header( socket_info* sock ){

	#if WEBSERVER_MAX_POST_CONTENT_LENGTH > UINT64_MAX
		#error WEBSERVER_MAX_POST_CONTENT_LENGTH > UINT64_MAX
	#endif

	if ( sock->header->contentlenght > WEBSERVER_MAX_POST_CONTENT_LENGTH ){
		LOG(CONNECTION_LOG,ERROR_LEVEL,sock->socket,"header->contentlenght > WEBSERVER_MAX_POST_CONTENT_LENGTH ( %d > %d ) ",sock->header->contentlenght, WEBSERVER_MAX_POST_CONTENT_LENGTH );
		return -1;
	}

	if ( sock->header->contenttype == 0 ){
		LOG(CONNECTION_LOG,ERROR_LEVEL,sock->socket,"header->contenttype == 0 ","");
		LOG(CONNECTION_LOG,ERROR_LEVEL,sock->socket,"%s",sock->header_buffer);
		return -1;
	}

	if ( ( sock->header->contenttype == MULTIPART_FORM_DATA ) && ( sock->header->boundary == 0 ) ){
		LOG(CONNECTION_LOG,ERROR_LEVEL,sock->socket,"header->boundary == 0 ","");
		LOG(CONNECTION_LOG,ERROR_LEVEL,sock->socket,"%s",sock->header_buffer);
		return -1;
	}

	return 0;
}

/*
 *
 * Return Values :
 *
 * 		0 = Header noch nicht zuende
 * 		1 = Header zuende aber noch Daten vorhanden
 *	   -1 = Fehler beim Header empfangen
 *
 */

int handleClientHeaderData(socket_info* sock) {
	int len2;
	unsigned int buffer_length = WEBSERVER_MAX_HEADER_LINE_LENGHT * 1;
	unsigned int parsed;

	unsigned int this_post_read = 0;


	if (sock->header_buffer == 0) {
		sock->header_buffer = (char*) WebserverMalloc( buffer_length );
		sock->header_buffer_pos = 0;
	}

	while (1) {

		if (buffer_length - sock->header_buffer_pos > 0) {
			int len = WebserverRecv(sock, (unsigned char*) &sock->header_buffer[sock->header_buffer_pos],
					buffer_length - sock->header_buffer_pos, 0);
			#ifdef _WEBSERVER_SOCKET_DEBUG_
				LOG(CONNECTION_LOG,NOTICE_LEVEL,sock->socket,"Client Header Recv %d",len);
			#endif

			if ((len == CLIENT_NO_MORE_DATA) && (sock->header_buffer_pos == 0)) {
				return 0;
			}

			if ((len == CLIENT_DISCONNECTED) || (len == CLIENT_UNKNOWN_ERROR) || (len == SSL_PROTOCOL_ERROR)) {
#if _WEBSERVER_CONNECTION_DEBUG_ > 1
				LOG ( CONNECTION_LOG,NOTICE_LEVEL,sock->socket,"Client Disconnected","" );
#endif
				sock->closeSocket = 1;
				return -1;
			}
			if ( sock->header->method == HTTP_POST ) {
				/*
				LOG(CONNECTION_LOG,NOTICE_LEVEL,sock->socket,"POST %d of %d",sock->header->post_buffer_pos,sock->header->contentlenght);
				*/
				if ( sock->header->contentlenght > sock->header->post_buffer_pos ) {
					if ( len == 0 ){
						return 0;
					}
					if ( 1 == recv_post_payload( sock ,sock->header_buffer, len ) ){
						return 1;	/* Aktuellen POST Request  gelesen, weiter Daten sind der nächste Header */
					}
				}
				this_post_read+=len;
				if ( this_post_read > 20000 ){
					return 0;
				}
				#warning "ändern weil so der WDT nicht mehr rankommt"
				continue;
			}

			sock->header_buffer_pos += len;
		}



		len2 = ParseHeader(sock, sock->header, sock->header_buffer, sock->header_buffer_pos, &parsed);
		if (len2 > 0) {
			reCopyHeaderBuffer(sock, len2);
			continue;
		}

		/* Header zuende aber noch weitere daten vorhanden */
		if (len2 == -3) {
			if ( sock->header->method == HTTP_POST ){
				if ( -1 == check_post_header( sock ) ){
					sock->header->error = 1;
					sendMethodBadRequest(sock);
					LOG ( HEADER_PARSER_LOG,NOTICE_LEVEL,sock->socket,"POST Method Header Error","" );
					return -1;
				}
				if ( 1 == recv_post_payload( sock, &sock->header_buffer[parsed + 1], sock->header_buffer_pos - ( parsed + 1 ) )){
					return 1; /* Aktuellen POST Request  gelesen, weiter Daten sind der nächste Header */
				}
				continue;
			}
			reCopyHeaderBuffer(sock, parsed + 1);
			return 1;
		}

		/* Header ist zuende und keine weiteren daten */
		if (len2 == -2){
			sock->header_buffer_pos = 0;
			sock->header_buffer[0] = '\0';
			if ( sock->header->method == HTTP_POST ){
				if ( -1 == check_post_header( sock ) ){
					sock->header->error = 1;
					sendMethodBadRequest(sock);
					LOG ( HEADER_PARSER_LOG,NOTICE_LEVEL,sock->socket,"POST Method Header Error","" );
					return -1;
				}

				/* Header ist zuende aber der POST Payload fehlt noch */
				return 0;
			}
#if _WEBSERVER_CONNECTION_DEBUG_ > 4
			LOG ( HEADER_PARSER_LOG,NOTICE_LEVEL,sock->socket,"Client Header Complete","" );
#endif
			return 1;
		}
		if (len2 == 0) {
			sendMethodBadRequestLineToLong(sock);
			LOG ( HEADER_PARSER_LOG,NOTICE_LEVEL,sock->socket,"sendMethodBadRequestLineToLong","" );
			sock->closeSocket = 1;
			return -1;
		}
		if (len2 == -4) {
			sendMethodNotAllowed(sock);
			LOG ( HEADER_PARSER_LOG,NOTICE_LEVEL,sock->socket,"sendMethodNotAllowed","" );
			sock->closeSocket = 1;
			return -1;
		}
	}
	return -1;
}

int checkConnectionKeepAlive(socket_info *sock) {

	if (sock->header->Connection != 0) {
		if (strcasecmp(sock->header->Connection, "keep-alive") != 0) {
			return -1;
		} else {
			WebserverResetHttpRequestHeader(sock->header);
			return 1;
		}
	}

	if (sock->header->isHttp1_1 == 1) {
		WebserverResetHttpRequestHeader(sock->header);
		return 1;
	}
	return -1;
}

int handleClient(socket_info* sock) {
	int ret;

#ifdef _WEBSERVER_SOCKET_DEBUG_
	LOG ( CONNECTION_LOG,NOTICE_LEVEL,sock->socket,"Client Data","" );
#endif
#ifdef WEBSERVER_USE_SSL
	if (sock->run_ssl_accept == 1) {
		ret = WebserverSSLAccept(sock);
		if (ret == CLIENT_NO_MORE_DATA) {
			addEventSocketRead(sock);
			return 0;
		}
		if (ret == SSL_ACCEPT_OK) {
#ifdef _WEBSERVER_SOCKET_DEBUG_
			LOG ( CONNECTION_LOG,NOTICE_LEVEL,sock->socket,"Handle SSL Data" ,"");
#endif
		}
		if (ret == SSL_PROTOCOL_ERROR) {
#ifdef _WEBSERVER_SOCKET_DEBUG_
			LOG ( CONNECTION_LOG,NOTICE_LEVEL,sock->socket,"SSL Protocol Error" ,"");
#endif
			return -1;
		}
		if (ret == NO_SSL_CONNECTION_ERROR) {
			return -1;
		}
	}
#endif
	ret = handleClientHeaderData(sock);
#if _WEBSERVER_CONNECTION_DEBUG_ > 4
	LOG ( CONNECTION_LOG,NOTICE_LEVEL,sock->socket,"handleClientData ret : %d",ret );
#endif

	/* das hier tritt auf wenn der header noch nicht zuende ist */
	if (ret == 0) {
		addEventSocketRead(sock);
		return 0;
	}
	if (ret == 1) {
#ifdef WEBSERVER_USE_WEBSOCKETS
		if ( sock->header->isWebsocket == 1 )
		{
			#if _WEBSERVER_CONNECTION_DEBUG_ > 1
				LOG ( CONNECTION_LOG,NOTICE_LEVEL,sock->socket,"Websocket Connected","" );
			#endif
			initWebsocketStructures(sock);
			sock->isWebsocket = 1;
			startWebsocketConnection ( sock );
			addEventSocketReadWritePersist ( sock );
			return 0;
		}

		/* Websocket Protokol Version wird nicht unterstützt */
		if ( sock->header->isWebsocket == 2 )
		{
			#if _WEBSERVER_CONNECTION_DEBUG_ > 2
				LOG ( CONNECTION_LOG,NOTICE_LEVEL,sock->socket,"isWebsocket 2","" );
			#endif
			generateOutputBuffer(sock);
			addEventSocketWritePersist(sock);
			return 0;
		}

#endif

#if _WEBSERVER_HANDLER_DEBUG_ > 4
		LOG ( HANDLER_LOG,NOTICE_LEVEL,sock->socket,"Handle Web Request","" );
#endif
		/* das hier tritt auf wenn mehrer requests ueber einen Socket im Burst gesendet werden */
		if (handleWebRequest(sock) < 0) {
			return -1;
		}
		sock->file_infos.file_send_pos = 0;

		generateOutputBuffer(sock);

		addEventSocketWritePersist(sock);

	}

	if (ret == -1) {
		return -1;
	}

	return 0;
}


char sendData(socket_info* sock, const unsigned char* buffer, FILE_OFFSET length) {
	int ret;
	int to_send;
	SOCKET_SEND_STATUS status;

	while (sock->file_infos.file_send_pos < length) {
		to_send = length - sock->file_infos.file_send_pos;
		if (to_send > WRITE_DATA_SIZE) to_send = WRITE_DATA_SIZE;

		status = WebserverSend(sock, &buffer[sock->file_infos.file_send_pos], to_send, 0, &ret);
#if _WEBSERVER_CONNECTION_DEBUG_ > 4
		LOG ( CONNECTION_LOG,ERROR_LEVEL,sock->socket,"%d of %d %d",ret,to_send,status );
#endif
		switch (status) {
		case SOCKET_SEND_NO_MORE_DATA:
			sock->file_infos.file_send_pos += ret;
			break;

		case SOCKET_SEND_CLIENT_DISCONNECTED:
			return CLIENT_DICONNECTED;

		case SOCKET_SEND_SEND_BUFFER_FULL:
			return DATA_PENDING;

		case SOCKET_SEND_SSL_ERROR:
			return CLIENT_DICONNECTED;

		default:
			LOG(CONNECTION_LOG, ERROR_LEVEL, sock->socket, "send error not handled %d", sock->socket, status);
			return CLIENT_DICONNECTED;
		}
	}

	return CLIENT_NO_MORE_DATA;
}

CLIENT_WRITE_DATA_STATUS handleClientWriteDataSendOutputBuffer(socket_info* sock) {
	int ret;

#ifdef _WEBSERVER_CONNECTION_SEND_DEBUG_
	LOG ( CONNECTION_LOG,ERROR_LEVEL,sock->socket,"handleClientWriteData Send Output Buffer ","" );
#endif

	ret = sendData(sock, (unsigned char*) sock->output_buffer, sock->output_buffer_size);
	switch (ret) {
	case CLIENT_NO_MORE_DATA:
		break;
	case DATA_PENDING:
		return DATA_PENDING;
	case CLIENT_DICONNECTED:
		WebserverFree(sock->output_buffer);
		sock->file_infos.file_send_pos = 0;
		return CLIENT_DICONNECTED;
	default:
		WebserverFree(sock->output_buffer);
		sock->file_infos.file_send_pos = 0;
		LOG(CONNECTION_LOG, ERROR_LEVEL, sock->socket, "unhandled send status output_buffer pos : %d status : %d",
				sock->file_infos.file_send_pos, ret);
		return CLIENT_DICONNECTED;
	}
	WebserverFree(sock->output_buffer);

	sock->output_buffer = 0;
	sock->file_infos.file_send_pos = 0;
	return NO_MORE_DATA;
}

CLIENT_WRITE_DATA_STATUS handleClientWriteDataSendRamFile(socket_info* sock) {
	int ret;
	ret = sendData(sock, sock->file_infos.file_info->Data, sock->file_infos.file_info->DataLenght);
	switch (ret) {
	case CLIENT_NO_MORE_DATA:
		break;
	case DATA_PENDING:
		return DATA_PENDING;
	case CLIENT_DICONNECTED:
		sock->file_infos.file_send_pos = 0;
		sock->file_infos.file_info = 0;
		return CLIENT_DICONNECTED;
	default:
		sock->file_infos.file_send_pos = 0;
		sock->file_infos.file_info = 0;
		LOG(CONNECTION_LOG, ERROR_LEVEL, sock->socket, "unhandled send status file ram pos : %d status : %d",
				sock->file_infos.file_send_pos, ret);
		return CLIENT_DICONNECTED;
	}
	sock->file_infos.file_send_pos = 0;
	sock->file_infos.file_info = 0;
	return NO_MORE_DATA;
}

CLIENT_WRITE_DATA_STATUS handleClientWriteDataSendFileSystem_sendfile(socket_info* sock) {
	off_t offset;
	int fd, diff, send;

	offset = sock->file_infos.file_send_pos;

	//printf("handleClientWriteDataSendFileSystem_sendfile : %s\n",sock->file_infos.file_info->Url);

	// TODO : an fs anpassen

	fd = PlatformOpenDataReadStream(sock->file_infos.file_info->FilePath);
	diff = sock->file_infos.file_info->DataLenght - sock->file_infos.file_send_pos;

	//printf("handleClientWriteDataSendFileSystem_sendfile: Diff %d DataLenght %d\n", diff, sock->file_infos.file_info->DataLenght );

	send = sendfile(sock->socket, fd, &offset, diff);

	PlatformCloseDataStream();

	if (send > 0) {
		sock->file_infos.file_send_pos += send;

		if (sock->file_infos.file_info->DataLenght > sock->file_infos.file_send_pos){
			 return DATA_PENDING;
		}

		sock->file_infos.file_send_pos = 0;
		sock->file_infos.file_info = 0;
		return NO_MORE_DATA;
	}

	if (send == -1) {
		switch (errno) {
		/* Nonblocking I/O has been selected using O_NONBLOCK and the write would block. */
		case EAGAIN:
			return DATA_PENDING;
			/* The input file was not opened for reading or the output file was not opened for writing. */
		case EBADF:
			return CLIENT_DICONNECTED;
			/* Bad address. */
		case EFAULT:
			return CLIENT_DICONNECTED;
			/* Descriptor is not valid or locked, or an mmap(2)-like operation is not available for in_fd. */
		case EINVAL:
			return CLIENT_DICONNECTED;
			/* Unspecified error while reading from in_fd. */
		case EIO:
			return CLIENT_DICONNECTED;
			/* Insufficient memory to read from in_fd. */
		case ENOMEM:
			return CLIENT_DICONNECTED;
		}
	}

	return CLIENT_DICONNECTED;
}

CLIENT_WRITE_DATA_STATUS handleClientWriteDataNotCached(socket_info* sock) {
	if (sock->use_ssl == 1) {
		return handleClientWriteDataNotCachedReadWrite(sock);
	} else {
#ifdef LINUX
		return handleClientWriteDataSendFileSystem_sendfile(sock);
#else
		return handleClientWriteDataNotCachedReadWrite(sock);
#endif
	}
}

CLIENT_WRITE_DATA_STATUS handleClientWriteData(socket_info* sock) {
	CLIENT_WRITE_DATA_STATUS status_ret;

	if (sock->output_buffer != 0) {
		status_ret = handleClientWriteDataSendOutputBuffer(sock);
		if (status_ret != NO_MORE_DATA) return status_ret;
	}

	if (sock->file_infos.file_info == 0) {
		return NO_MORE_DATA;
	}

#ifdef _WEBSERVER_CONNECTION_SEND_DEBUG_
	LOG ( CONNECTION_LOG,ERROR_LEVEL,sock->socket,"handleClientWriteData Send File  Bytes %ld Pos %ld", sock->file_infos.file_info->DataLenght,sock->file_infos.file_send_pos );
#endif

	if (sock->file_infos.file_info->RamCached == 1) {
		return handleClientWriteDataSendRamFile(sock);
	} else {

		return handleClientWriteDataNotCached(sock);

	}

	return NO_MORE_DATA;
}



void handleServer(socket_info* sock) {
	int c;
	unsigned int port;
	socket_info *client_sock;

	while (1) {
		c = PlatformAccept(sock, &port);
		if (c == -1) return;

#ifdef WEBSERVER_USE_WNFS
		if (sock->server == 2) {
			if ( wnfs_socket != 0 )
				close( wnfs_socket );
			wnfs_socket = c;

			LOG ( CONNECTION_LOG,NOTICE_LEVEL,c,"WNFS Connection from %s",sock->client_ip_str );

			/*client_sock = addClientSocket(c, 0);
			strncpy(client_sock->client_ip_str, sock->client_ip_str, INET_ADDRSTRLEN);
			client_sock->port = port;*/
			continue;
		}
#endif

		if (sock->use_ssl == 1) {
#ifdef WEBSERVER_USE_SSL
#if _WEBSERVER_CONNECTION_DEBUG_ > 1
			LOG ( CONNECTION_LOG,NOTICE_LEVEL,c,"SSL Connection from %s",sock->client_ip_str );
#endif
			PlatformSetNonBlocking(c);
			client_sock = addClientSocket(c, 1);
			client_sock->run_ssl_accept = 1;
			WebserverSSLInit(client_sock);
#endif

		} else {
			PlatformSetNonBlocking(c);
#if _WEBSERVER_CONNECTION_DEBUG_ > 1
			LOG ( CONNECTION_LOG,NOTICE_LEVEL,c,"Normal Connection from %s",sock->client_ip_str );
#endif
			client_sock = addClientSocket(c, 0);
		}
		strncpy(client_sock->client_ip_str, sock->client_ip_str, INET_ADDRSTRLEN);
		client_sock->port = port;

#ifdef WEBSERVER_USE_IPV6
		client_sock->v6_client = sock->v6_client;
#endif
	}

}

int WebserverCloseSocket(socket_info* s) {

#ifdef WEBSERVER_USE_WEBSOCKETS
	if ( s->isWebsocket == 1 ) {
#if _WEBSERVER_CONNECTION_DEBUG_ > 1
		LOG( CONNECTION_LOG, NOTICE_LEVEL, s->socket, "Closing Websocket Connection", "");
#endif
	}else{
#endif
#if _WEBSERVER_CONNECTION_DEBUG_ > 1
		LOG( CONNECTION_LOG, NOTICE_LEVEL, s->socket, "Closing Client Connection", "");
#endif
#ifdef WEBSERVER_USE_WEBSOCKETS
	}
#endif

#ifdef WEBSERVER_USE_SSL
	if (s->use_ssl == 1) {
		WebserverSSLCloseSockets(s);
		return PlatformCloseSocket(s->socket);
	} else {
		return PlatformCloseSocket(s->socket);
	}
#else
	return PlatformCloseSocket(s->socket);
#endif
}

CLIENT_WRITE_DATA_STATUS handleClientWriteDataNotCachedReadWrite(socket_info* sock) {
	int ret;
	CLIENT_WRITE_DATA_STATUS ret_v;
	SOCKET_SEND_STATUS status;
	unsigned int to_read, diff;
	unsigned char *buffer;

#ifdef _WEBSERVER_FILESYSTEM_DEBUG_
		LOG ( CONNECTION_LOG,NOTICE_LEVEL,sock->socket,"Send File from Disk Soll %d Pos %d",sock->file_infos.file_info->DataLenght,sock->file_infos.file_send_pos );
#endif

	buffer = (unsigned char *) WebserverMalloc( WRITE_DATA_SIZE );
	PlatformOpenDataReadStream(sock->file_infos.file_info->FilePath);
	PlatformSeekToPosition(sock->file_infos.file_send_pos);

	while (1) {
		diff = sock->file_infos.file_info->DataLenght - sock->file_infos.file_send_pos;
		if (diff > WRITE_DATA_SIZE)
			to_read = WRITE_DATA_SIZE;
		else
			to_read = diff;


		FILE_OFFSET ret2 = PlatformReadBytes(buffer, to_read);
		if ( ret2 != to_read ){
			printf("Error: read mismatch %jd != %d\n",ret2,to_read);
		}

		status = WebserverSend(sock, buffer, to_read, 0, &ret);

		switch (status) {
		case SOCKET_SEND_NO_MORE_DATA:
#if _WEBSERVER_CONNECTION_DEBUG_ >= 5
			LOG ( SOCKET_LOG,NOTICE_LEVEL,sock->socket,"SOCKET_SEND_NO_MORE_DATA : %d status : %d ret : %d",sock->file_infos.file_send_pos,status,ret );
#endif
			sock->file_infos.file_send_pos += ret;
			if (sock->file_infos.file_info->DataLenght == sock->file_infos.file_send_pos) {
				ret_v = NO_MORE_DATA;
				goto send_ende;
			}

			break;

		case SOCKET_SEND_CLIENT_DISCONNECTED:
			ret_v = CLIENT_DICONNECTED;
			goto send_ende;

		case SOCKET_SEND_SEND_BUFFER_FULL:
			sock->file_infos.file_send_pos += ret;
			PlatformCloseDataStream();
			WebserverFree(buffer);
			return DATA_PENDING;

		default:
			LOG(SOCKET_LOG, ERROR_LEVEL, sock->socket, "unhandled send status file file pos : %d status : %d",
					sock->file_infos.file_send_pos, status);
			ret_v = CLIENT_DICONNECTED;
			goto send_ende;
		}

		if (sock->file_infos.file_info->DataLenght == sock->file_infos.file_send_pos) {
			ret_v = NO_MORE_DATA;
			break;
		}
	}

	send_ende: sock->file_infos.file_info = 0;
	PlatformCloseDataStream();
	WebserverFree(buffer);
	return ret_v;
}



void WebserverConnectionManagerStartLoop(void) {

	initSocketContainer();

	while (1) {

		if (WebserverStartConnectionManager() < 0) {
			LOG(CONNECTION_LOG, ERROR_LEVEL, 0, "Error : ConnectionManager konnte nicht gestartet werden", "");
			return;
		}

		waitEvents();
		return;
	}

}

void handleer( int a, short b, void *t ) {
	int ret;
	socket_info* sock = (socket_info*) t;
	CLIENT_WRITE_DATA_STATUS status_ret = UNDEFINED;


#if _WEBSERVER_HANDLER_DEBUG_ > 3

	if ( b == EVENT_TIMEOUT ){ LOG ( HANDLER_LOG,NOTICE_LEVEL,sock->socket,"EV_TIMEOUT","" ); }
	if ( b == EVENT_READ ) {   LOG ( HANDLER_LOG,NOTICE_LEVEL,sock->socket,"EV_READ","" );    }
	if ( b == EVENT_WRITE ) {  LOG ( HANDLER_LOG,NOTICE_LEVEL,sock->socket,"EV_WRITE","" );   }

#endif

	if ( b == EVENT_SIGNAL ) {
		LOG( MESSAGE_LOG, NOTICE_LEVEL,sock->socket,"EV_SIGNAL","" );
		LOG( MESSAGE_LOG, ERROR_LEVEL, sock->socket,"EV_SIGNAL not handled","%d", b );
		return;
	}

	if ( a != sock->socket ){
		LOG( MESSAGE_LOG, ERROR_LEVEL, sock->socket,"a != sock->socket","%d", b );
		return;
	}

	if ( sock->extern_handle != 0 )
	{
		sock->extern_handle(sock->socket, sock->extern_handle_data_ptr);
		return;
	}

#ifdef WEBSERVER_USE_WEBSOCKETS
	if ( sock->isWebsocket == 1 )
	{
		websocket_event_handler(sock,(EVENT_TYPES)b);
		return;
	}
#endif

	if (sock->closeSocket == 1) {
		WebserverConnectionManagerCloseRequest(sock);
		return;
	}

	if (sock->server == 1) {
		handleServer(sock);
		return;
	}

	if (sock->server == 2) {
		handleServer(sock);
		return;
	}

	if (sock->client == 1) {
		if (b == EVENT_READ) {
			#ifdef WEBSERVER_USE_SSL
			sock->ssl_pending = 0;
			if(sock->use_ssl == 1){


				/*
  				 Im SSL read muss die event registrierung blockiert werden bis
				 alle pending bytes gelesen wurden
				*/
				sock->ssl_block_event_flags = 1;
				ret = handleClient(sock);
				if (ret < 0) {
					WebserverConnectionManagerCloseRequest(sock);
					return;
				}
				while ( WebserverSSLPending ( sock ) ){
					sock->ssl_pending = 1;
					ret = handleClient(sock);
					if (ret < 0) {
						WebserverConnectionManagerCloseRequest(sock);
						return;
					}
				}
				sock->ssl_pending = 0;
				sock->ssl_block_event_flags = 0;

				commitSslEventFlags( sock );

				return;
			}else{
			#endif
				ret = handleClient(sock);
				if (ret < 0) {
					WebserverConnectionManagerCloseRequest(sock);
					return;
				}

				return;
			#ifdef WEBSERVER_USE_SSL
			}
			#endif

		}
		if (b == EVENT_WRITE) {
			status_ret = handleClientWriteData(sock);
			switch (status_ret) {
			case NO_MORE_DATA:
#if _WEBSERVER_CONNECTION_DEBUG_ > 4
				LOG ( CONNECTION_LOG,NOTICE_LEVEL,sock->socket,"request finished","" );
#endif
				delEventSocketWritePersist(sock);
				if (checkConnectionKeepAlive(sock) == 1) {
					if (sock->header_buffer_pos > 0) {
						if (sock->closeSocket == 1) {
							WebserverConnectionManagerCloseRequest(sock);
							return;
						} else {
							handleClient(sock);
							return;
						}
					} else {
						if (sock->closeSocket == 1) {
							WebserverConnectionManagerCloseRequest(sock);
							return;
						}
						addEventSocketRead(sock);
						return;
					}
				}

				#ifdef WEBSERVER_USE_WEBSOCKETS
				if( sock->header->isWebsocket == 2 ){
					addEventSocketRead(sock);
					return;
				}
				#endif

				WebserverConnectionManagerCloseRequest(sock);
				return;

			case DATA_PENDING:
				break;
			case CLIENT_DICONNECTED:
#ifdef _WEBSERVER_CONNECTION_DEBUG_
				LOG ( CONNECTION_LOG,WARNING_LEVEL,sock->socket,"request finished client disconnected","" );
#endif
				delEventSocketWritePersist(sock);
				WebserverConnectionManagerCloseRequest(sock);
				return;
			case UNDEFINED:
				LOG(CONNECTION_LOG, ERROR_LEVEL, sock->socket, "UNDEFINED STATUS", "");
				return;
			}
		}
		return;
	}
}

unsigned long getSocketInfoSize(socket_info* sock) {
	unsigned long ret = sizeof(socket_info);

	if (sock->header_buffer != 0) ret += WEBSERVER_MAX_HEADER_LINE_LENGHT + 1; /* header_buffer */
	if (sock->header != 0) ret += sizeof(HttpRequestHeader); /* header */
#ifdef WEBSERVER_USE_WEBSOCKETS
	if(sock->websocket_buffer != 0)
		ret+= WebserverMallocedSize(sock->websocket_buffer);

	if(sock->websocket_guid != 0)
		ret+= WEBSERVER_GUID_LENGTH+1;
#endif
	ret += sock->output_buffer_size;

	return ret;
}

