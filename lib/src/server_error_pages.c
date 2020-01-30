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

int sendMethodNotAllowed(socket_info *sock){
	int length = 0;
	sock->error_requests_on_socket++;
	length += printHTMLChunk( sock,"<html><title>405 Method Not Allowed</title><body>");
	length += printHTMLChunk( sock,"<h1>405 Method Not Allowed</h1>");
	length += printHTMLChunk( sock,"<p>The Method is not allowed on this server.</p><hr />");
	length += printAddressInfo( sock);
	sendHeaderError( sock,(char*)"405 Method Not Allowed",length);
	return 0;
}

int sendMethodBadRequest(socket_info *sock){
	int length = 0;
	sock->error_requests_on_socket++;
	length += printHTMLChunk(sock,"<html><title>400 Bad Request</title>");
	length += printHTMLChunk(sock,"<body><h1>400 Bad Request</h1>");
	length += printHTMLChunk(sock,"<p>The request could not be understood by the server due to malformed syntax.</p><hr />");
	length += printAddressInfo(sock);
	sendHeaderError(sock,(char*)"400 Bad Request",length);
	return 0;
}

int sendMethodBadRequestMissingHeaderLines(socket_info *sock){
	int length = 0;
	sock->error_requests_on_socket++;
	length += printHTMLChunk(sock,"<html><title>400 Bad Request</title>");
	length += printHTMLChunk(sock,"<body><h1>400 Bad Request</h1>");
	length += printHTMLChunk(sock,"<p>The request could not be understood by the server due to malformed syntax.");
	length += printHTMLChunk(sock,"Header Lines missing.</p><hr />");
	length += printAddressInfo(sock);
	sendHeaderError(sock,(char*)"400 Bad Request",length);
	return 0;
}

int sendMethodBadRequestLineToLong(socket_info *sock){
	int length = 0;
	sock->error_requests_on_socket++;
	length += printHTMLChunk(sock,"<html><title>400 Bad Request</title>");
	length += printHTMLChunk(sock,"<body><h1>400 Bad Request</h1>");
	length += printHTMLChunk(sock,"<p>The request could not be understood by the server due to malformed syntax.");
	length += printHTMLChunk(sock,"The URL is to long.</p><hr />");
	sendHeaderError(sock,(char*)"400 Bad Request",length);
	return 0;
}


