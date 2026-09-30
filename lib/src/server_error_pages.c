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


static int printAddressInfo(socket_info *sock){
	int length = 0;
	length += printHTMLChunk(sock,"<address>%s at %s ",SERVER_NAME,SERVER_DOMAIN);

#ifdef WEBSERVER_USE_SSL
	if(sock->use_ssl == 1){
		length += printHTMLChunk(sock,"Port %d</address></body></html>",getConfigInt("ssl_port"));
	}else{
		length += printHTMLChunk(sock,"Port %d</address></body></html>",getConfigInt("port"));
	}
#else
	
	length += printHTMLChunk(sock, "Port %d</address></body></html>", getConfigInt("port"));
	
#endif
	return length;
}

int sendFileNotFound(http_request *s){
	int length = 0;
	length += printHTMLChunk(s->socket,"<html><title>404 Not Found</title><body><h1>Not Found</h1><p>");
	length += printHTMLChunk(s->socket,"The requested URL was not found on this server.</p><hr />");
	length += printAddressInfo(s->socket);
	sendHeaderNotFound(s,length);
	return 0;
}

int sendAccessDenied(http_request* s){
	int length = 0;
	length += printHTMLChunk( s->socket,"<html><title>403 Forbidden</title><body>");
	length += printHTMLChunk( s->socket,"<h1>403 Forbidden</h1>");
	length += printHTMLChunk( s->socket,"<p>Your client does not have permission to access file.</p><hr />");
	length += printAddressInfo( s->socket);
	sendHeaderError( s->socket,(char*)"403 Forbidden",length);
	return 0;
}

/*
 * The pages below all look the same - only the status line and one sentence
 * differ. They used to be one function per status, and that is how a missing
 * status stayed missing: the size check in check_post_header() had no sender
 * of its own, reused the 400 helper, and an upload above the limit was
 * therefore reported as malformed syntax. With the status as a parameter a
 * new one costs a table row. reverse_proxy.c already works this way, see
 * proxy_send_error_response().
 *
 * Only the statuses that abort the request belong here: they all count
 * towards error_requests_on_socket and close the connection. 403 and 404 are
 * regular answers on a connection that stays open and keep their own senders.
 */
typedef struct {
	int code;
	const char* reason;
	const char* description;
} http_error_page;

static const http_error_page http_error_pages[] = {
	{ 400, "Bad Request",
	       "The request could not be understood by the server due to malformed syntax." },
	{ 405, "Method Not Allowed",
	       "The Method is not allowed on this server." },
	{ 413, "Request Entity Too Large",
	       "The request body is larger than this server accepts for this URL." },
	{ 500, "Internal Server Error",
	       "The request could not be handled." },
	/* The request is fine, the server is just out of memory to buffer the
	 * body right now. 503 rather than 4xx: the cause is on our side and the
	 * client may retry later. */
	{ 503, "Service Unavailable",
	       "The server is temporarily unable to buffer the request body." },
};

static const http_error_page* findErrorPage( int code ){
	size_t i;

	for ( i = 0; i < ( sizeof( http_error_pages ) / sizeof( http_error_pages[0] ) ); i++ ){
		if ( http_error_pages[i].code == code ){
			return &http_error_pages[i];
		}
	}
	return 0;
}

/*
 * Send an error page and end the request.
 *
 * detail is appended to the standard sentence and may be 0.
 *
 * closeSocket is set here, before the header is built: addConnectionStatusLines()
 * reads it to announce "Connection: close". The announcement has to match what
 * happens next - a client that is told keep-alive on a connection the server
 * closes right afterwards treats the answer as broken. Setting it here also
 * matches what every caller does anyway, they all end the request.
 */
int sendHttpErrorPage(socket_info *sock, int code, const char* detail){
	const http_error_page* page = findErrorPage( code );
	char status_line[64];
	int length = 0;

	/* An unknown status would leave an empty status line, which no client can
	 * parse. 500 is the honest answer: the caller asked for something this
	 * server cannot phrase. */
	if ( page == 0 ){
		LOG( CONNECTION_LOG, ERROR_LEVEL, sock->socket, "no error page for status %d, sending 500", code );
		page = findErrorPage( 500 );
		if ( page == 0 ){
			return -1;
		}
	}

	sock->error_requests_on_socket++;
	sock->closeSocket = 1;

	snprintf( status_line, sizeof( status_line ), "%d %s", page->code, page->reason );

	length += printHTMLChunk(sock,"<html><title>%s</title>", status_line);
	length += printHTMLChunk(sock,"<body><h1>%s</h1>", status_line);
	length += printHTMLChunk(sock,"<p>%s", page->description);
	if ( detail != 0 ){
		length += printHTMLChunk(sock," %s", detail);
	}
	length += printHTMLChunk(sock,"</p><hr />");
	length += printAddressInfo(sock);

	sendHeaderError(sock, status_line, length);
	return 0;
}



