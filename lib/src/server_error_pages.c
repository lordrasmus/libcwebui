/*

libCWebUI
Copyright (C) 2016  Ramin Seyed-Moussavi

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

#include "webserver.h"

#ifdef DMALLOC
#include <dmalloc/dmalloc.h>
#endif


int printAddressInfo(socket_info *socket){
	int length = 0;
	length += printHTMLChunk(socket,"<address>%s at %s ",SERVER_NAME,SERVER_DOMAIN);
	if(socket->use_ssl == 1)
		length += printHTMLChunk(socket,"Port %d</address></body></html>",getConfigInt("ssl_port"));
	else
		length += printHTMLChunk(socket,"Port %d</address></body></html>",getConfigInt("port"));
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

int sendMethodNotAllowed(socket_info *socket){
	int length = 0;
	length += printHTMLChunk( socket,"<html><title>405 Method Not Allowed</title><body>");
	length += printHTMLChunk( socket,"<h1>405 Method Not Allowed</h1>");
	length += printHTMLChunk( socket,"<p>The Method is not allowed on this server.</p><hr />");
	length += printAddressInfo( socket);
	sendHeaderError( socket,(char*)"405 Method Not Allowed",length);
	return 0;
}

int sendMethodBadRequest(socket_info *socket){
	int length = 0;
	length += printHTMLChunk(socket,"<html><title>400 Bad Request</title>");
	length += printHTMLChunk(socket,"<body><h1>400 Bad Request</h1>");
	length += printHTMLChunk(socket,"<p>The request could not be understood by the server due to malformed syntax.</p><hr />");
	length += printAddressInfo(socket);
	sendHeaderError(socket,(char*)"400 Bad Request",length);
	return 0;
}

int sendMethodBadRequestMissingHeaderLines(socket_info *socket){
	int length = 0;
	length += printHTMLChunk(socket,"<html><title>400 Bad Request</title>");
	length += printHTMLChunk(socket,"<body><h1>400 Bad Request</h1>");
	length += printHTMLChunk(socket,"<p>The request could not be understood by the server due to malformed syntax.");
	length += printHTMLChunk(socket,"Header Lines missing.</p><hr />");
	length += printAddressInfo(socket);
	sendHeaderError(socket,(char*)"400 Bad Request",length);
	return 0;
}

int sendMethodBadRequestLineToLong(socket_info *socket){
	int length = 0;
	length += printHTMLChunk(socket,"<html><title>400 Bad Request</title>");
	length += printHTMLChunk(socket,"<body><h1>400 Bad Request</h1>");
	length += printHTMLChunk(socket,"<p>The request could not be understood by the server due to malformed syntax.");
	length += printHTMLChunk(socket,"The URL is to long.</p><hr />");
	sendHeaderError(socket,(char*)"400 Bad Request",length);
	return 0;
}


