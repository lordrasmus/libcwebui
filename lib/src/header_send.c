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

#include "webserver.h"

#ifdef DMALLOC
#include <dmalloc/dmalloc.h>
#endif


// https://www.html5rocks.com/static/images/cors_server_flowchart.png

static cors_handler cors_handle_func = 0;

void setCORS_Handler( cors_handler handler ){
	cors_handle_func = handler;
	LOG(HEADER_PARSER_LOG,NOTICE_LEVEL,0,"%s","CORS Handler set");
}


static int checkCORS( CORS_HEADER_TYPES type, socket_info* socket ){

	if ( cors_handle_func == 0 ){
		return COND_FALSE;
	}

	cors_infos info;
	info.type = type;

	if ( socket->header->Origin != 0 ){
		info.origin = socket->header->Origin;
	}else{
		info.origin = "";
	}

	if ( socket->header->method == HTTP_OPTIONS ){
		info.method = "OPTIONS";
	}

	if ( socket->header->method == HTTP_GET ){
		info.method = "GET";
	}

	if ( socket->header->method == HTTP_POST ){
		info.method = "POST";
	}

	return cors_handle_func( &info );

}

char* get_cors_type_name( CORS_HEADER_TYPES type ){

	switch( type){
		case CORS_ALLOW_ORIGIN: return "CORS_ALLOW_ORIGIN";
		case CORS_ALLOW_CREDENTIALS: return "CORS_ALLOW_CREDENTIALS";
		case CORS_ALLOW_METHODS: return "CORS_ALLOW_METHODS";
		case CORS_ALLOW_HEADERS: return "CORS_ALLOW_HEADERS";
	}
	return "UNKNOWN";
}


#ifdef WEBSERVER_USE_WEBSOCKETS

int sendHeaderWebsocket(socket_info* sock) {
	
	if ( sock->header->isHttp1_1 ){
		printWebsocketChunk(sock, "HTTP/1.1 101 Switching Protocols\r\n");
	}else{
		printWebsocketChunk(sock, "HTTP/1.0 101 Switching Protocols\r\n");
	}
	printWebsocketChunk(sock, "Upgrade: websocket\r\n");
	printWebsocketChunk(sock, "Connection: Upgrade\r\n");
	printWebsocketChunk(sock, "Sec-WebSocket-Accept: %s\r\n", sock->header->WebSocketOutHash);
	printWebsocketChunk(sock, "\r\n");
	return 0;
}

#endif


static void addConnectionStatusLines(socket_info* socket) {

	if (socket->header->Connection != 0) {
		if (strcmp(socket->header->Connection, "close") == 0) {
			printHeaderChunk(socket, "Connection: close\r\n");
			return;
		}
		printHeaderChunk(socket, "Connection: %s\r\n",socket->header->Connection);
	} else {
		printHeaderChunk(socket, "Connection: close\r\n");
	}


	// Access-Control-Expose-Headers

	if ( ( socket->header->Origin != 0 ) && ( COND_TRUE == checkCORS( CORS_ALLOW_ORIGIN, socket ) ) ){
		printHeaderChunk(socket, "Access-Control-Allow-Origin: %s\r\n",socket->header->Origin);
	}

	if ( COND_TRUE == checkCORS( CORS_ALLOW_CREDENTIALS, socket ) ){
		printHeaderChunk( socket, "Access-Control-Allow-Credentials: true\r\n");
	}

}

int sendPreflightAllowed(socket_info *sock) {

	if ( sock->header->isHttp1_1 ){
		printHeaderChunk( sock, "HTTP/1.1 204 No Content\r\n");
	}else{
		printHeaderChunk( sock, "HTTP/1.0 204 No Content\r\n");
	}
	printHeaderChunk( sock, "Server: %s\r\n", "libCWebUI");


	if ( ( sock->header->Access_Control_Request_Method != 0 ) && ( COND_TRUE == checkCORS( CORS_ALLOW_METHODS, sock ) ) ){
		printHeaderChunk( sock, "Access-Control-Allow-Methods: %s\r\n",sock->header->Access_Control_Request_Method);
	}

	if ( ( sock->header->Access_Control_Request_Headers != 0 ) && ( COND_TRUE == checkCORS( CORS_ALLOW_HEADERS, sock ) ) ){
		printHeaderChunk( sock, "Access-Control-Allow-Headers: %s\r\n",sock->header->Access_Control_Request_Headers);
	}

	// Access-Control-Max-Age

	if ( ( sock->header->Origin != 0 ) && ( COND_TRUE == checkCORS( CORS_ALLOW_ORIGIN, sock ) ) ){
		printHeaderChunk( sock, "Access-Control-Allow-Origin: %s\r\n",sock->header->Origin);
	}

	if ( COND_TRUE == checkCORS( CORS_ALLOW_CREDENTIALS, sock ) ){
		printHeaderChunk( sock, "Access-Control-Allow-Credentials: true\r\n");
	}

	printHeaderChunk( sock, "\r\n"); /* HTTP Header beenden */

	return 1;
}

static void addCacheControlLines(http_request* s, WebserverFileInfo *info) {
	/* Cache-Control: no-cache, no-store, must-revalidate, pre-check=0, post-check=0 */
#ifndef WEBSERVER_DISABLE_CACHE
	if ( ( info->FileType != FILE_TYPE_HTML ) && ( info->TemplateFile == 0   ) ) {
		printHeaderChunk ( s->socket,"Cache-Control: max-age=%d, public\r\n",MAX_CACHE_AGE ); /* sekunden bis refresh */
		/* printHeaderChunk ( s->socket,"Expires: Thu, 15 Apr 2060 20:00:00 GMT\r\n"); */

		if ( info->etag != 0 ){
			printHeaderChunk ( s->socket,"ETag: %s\r\n",info->etag );
		}

		if ( info->lastmodified != 0 ){
			printHeaderChunk ( s->socket,"Last-Modified: %s\r\n",info->lastmodified );
		}
	}else{
		printHeaderChunk ( s->socket,"Cache-Control: no-cache, no-store, must-revalidate\r\n" );
	}
#else
	printHeaderChunk ( s->socket,"Cache-Control: no-cache, no-store, must-revalidate\r\n" );
#endif
}

int sendHeaderNotModified(http_request* s, WebserverFileInfo *info) {
	if ( s->header->isHttp1_1 ){
		printHeaderChunk(s->socket, "HTTP/1.1 304 Not Modified\r\n");
	}else{
		printHeaderChunk(s->socket, "HTTP/1.0 304 Not Modified\r\n");
	}
	printHeaderChunk(s->socket, "Server: %s\r\n", "libCWebUI");
	addCacheControlLines(s, info);
	addConnectionStatusLines(s->socket);
	printHeaderChunk(s->socket, "\r\n");
	return 0;
}

void sendHeaderNotFound(http_request* s, int p_lenght) {
	if ( s->header->isHttp1_1 ){
		printHeaderChunk(s->socket, "HTTP/1.1 404 Not Found\r\n");
	}else{
		printHeaderChunk(s->socket, "HTTP/1.0 404 Not Found\r\n");
	}
	printHeaderChunk(s->socket, "Server: %s\r\n", "libCWebUI");
	printHeaderChunk(s->socket, "Content-Length: %d\r\n", p_lenght);
	printHeaderChunk(s->socket, "Content-Type: text/html\r\n");
	addConnectionStatusLines(s->socket);
	printHeaderChunk(s->socket, "\r\n");
}

void sendHeaderError(socket_info* sock, char* ErrorMessage, int p_lenght) {
	if ( sock->header->isHttp1_1 ){
		printHeaderChunk(sock, "HTTP/1.1 %s\r\n", ErrorMessage);
	}else{
		printHeaderChunk(sock, "HTTP/1.0 %s\r\n", ErrorMessage);
	}
	printHeaderChunk(sock, "Server: %s\r\n", "libCWebUI");
	printHeaderChunk(sock, "Content-Length: %d\r\n", p_lenght);
	printHeaderChunk(sock, "Content-Type: text/html\r\n");
	addConnectionStatusLines(sock);
	printHeaderChunk(sock, "\r\n");
}

static void addContentTypeLines(http_request *s, WebserverFileInfo *info) {

	switch ( info->Compressed ){
		case 1 : printHeaderChunk(s->socket, "%s", "Content-Encoding: gzip\r\n"); break;
		case 2 : printHeaderChunk(s->socket, "%s", "Content-Encoding: deflate\r\n"); break;
	}

	/* http://wiki.selfhtml.org/wiki/Referenz:MIME-Typen
	 * http://www.sitepoint.com/web-foundations/mime-types-complete-list/
	 */
	switch (info->FileType) {
	case FILE_TYPE_PLAIN:
		printHeaderChunk(s->socket, "%s", "Content-Type: text/plain\r\n");
		break;

	case FILE_TYPE_HTML:
		printHeaderChunk(s->socket, "%s", "Content-Type: text/html\r\n");
		break;

	case FILE_TYPE_HTML_INC:
		printHeaderChunk(s->socket, "%s", "Content-Type: text/html\r\n");
		break;

	case FILE_TYPE_XML:
		printHeaderChunk(s->socket, "%s", "Content-Type: text/xml\r\n");
		break;

	case FILE_TYPE_XSL:
		printHeaderChunk(s->socket, "%s", "Content-Type: text/xsl\r\n");
		break;

	case FILE_TYPE_CSS:
		printHeaderChunk(s->socket, "%s", "Content-Type: text/css\r\n");
		break;

	case FILE_TYPE_JS:
		printHeaderChunk(s->socket, "%s", "Content-Type: text/javascript\r\n");
		break;

	case FILE_TYPE_JPG:
		printHeaderChunk(s->socket, "%s", "Content-Type: image/jpeg\r\n");
		break;

	case FILE_TYPE_PNG:
		printHeaderChunk(s->socket, "%s", "Content-Type: image/png\r\n");
		break;

	case FILE_TYPE_GIF:
		printHeaderChunk(s->socket, "%s", "Content-Type: image/gif\r\n");
		break;

	case FILE_TYPE_ICO:
		printHeaderChunk(s->socket, "%s", "Content-Type: image/x-icon\r\n");
		break;

	case FILE_TYPE_MANIFEST:
		printHeaderChunk(s->socket, "%s", "Content-Type: text/cache-manifest\r\n");
		break;

	case FILE_TYPE_SVG:
		printHeaderChunk(s->socket, "%s", "Content-Type: image/svg+xml\r\n");
		break;

	case FILE_TYPE_BMP:
		printHeaderChunk(s->socket, "%s", "Content-Type: image/bmp\r\n");
		break;

	case FILE_TYPE_JSON:
		/* if ( info->ForceDownload == 0){ */
			printHeaderChunk(s->socket, "%s", "Content-Type: application/json\r\n");
		/* } */
		break;

	case FILE_TYPE_PDF:
		printHeaderChunk(s->socket, "%s", "Content-Type: application/pdf\r\n");
		break;

	case FILE_TYPE_WOFF:
		printHeaderChunk(s->socket, "%s", "Content-Type: application/x-font-woff\r\n");
		break;

	case FILE_TYPE_EOT:
		printHeaderChunk(s->socket, "%s", "Content-Type: application/vnd.ms-fontobject\r\n");
		break;

	case FILE_TYPE_TTF:
		printHeaderChunk(s->socket, "%s", "Content-Type: application/x-font-ttf\r\n");
		break;

	case FILE_TYPE_C_SRC:
		printHeaderChunk(s->socket, "%s", "Content-Type: text/x-c\r\n");
		break;



	default:
		LOG(FILESYSTEM_LOG, ERROR_LEVEL, s->socket->socket, "Webserver Error : Unbekannter Filetyp %d", info->FileType);
		break;
	}
}

static void addCSPHeaderLines(http_request* s){

	char buff[1000];
	int offset = 0;

	if ( getConfigInt( "use_csp") == 0 ){
		return;
	}

	/* ; style-src 'self' ; img-src 'self' ; script-src 'self' */

#ifdef WEBSERVER_USE_SSL

	offset += snprintf(&buff[offset],1000-offset,"default-src http://%s https://%s; ",s->header->Host ,s->header->Host);
	offset += snprintf(&buff[offset],1000-offset,"script-src 'self' 'unsafe-eval' 'unsafe-inline' http://%s https://%s; ",s->header->Host ,s->header->Host);
	offset += snprintf(&buff[offset],1000-offset,"style-src 'unsafe-inline' http://%s https://%s; ",s->header->Host ,s->header->Host);
	          snprintf(&buff[offset],1000-offset,"connect-src ws://%s http://%s wss://%s https://%s ; ",s->header->Host ,s->header->Host,s->header->Host ,s->header->Host );

#else

	offset += snprintf(&buff[offset],1000-offset,"default-src http://%s; ",s->header->Host );
	offset += snprintf(&buff[offset],1000-offset,"script-src 'self' 'unsafe-eval' 'unsafe-inline' http://%s; ",s->header->Host );
	offset += snprintf(&buff[offset],1000-offset,"style-src 'unsafe-inline' http://%s; ",s->header->Host );
	          snprintf(&buff[offset],1000-offset,"connect-src ws://%s http://%s; ",s->header->Host ,s->header->Host );

#endif


	//offset += snprintf(&buff[offset],1000-offset," http://%s https://%s; ",s->header->Host ,s->header->Host);
/*
	printHeaderChunk(s->socket, "%s %s\r\n", "X-WebKit-CSP: ", buff);
	printHeaderChunk(s->socket, "%s %s\r\n", "X-Content-Security-Policy: ", buff);
*/
	printHeaderChunk(s->socket, "%s %s\r\n", "Content-Security-Policy: ",buff);


}

/*********************************************************************************
 *
 *   Probleme
 *
 *   mit HttpOnly wird im Chrome der Cookie nicht beim Websocket benutzt
 *   Konquerer scheint Path zu brauchen um die Cookies im AJAX Requests zu benutzen
 *
 *********************************************************************************/

static void addSessionCookies(http_request* s,WebserverFileInfo *info){
#ifdef WEBSERVER_USE_SESSIONS

	#ifdef WEBSERVER_USE_SSL

	if ((s->create_cookie == 1) && (info->FileType == FILE_TYPE_HTML)) { /* Nur bei HTML Seiten Session cookies senden */
		/*
		printHeaderChunk(s->socket, "Set-Cookie: session-id=%s; Version=\"1\"; Path=\"/\"; Discard; HttpOnly; domain=%s\r\n", s->guid,s->header->Host);
		printHeaderChunk(s->socket, "Set-Cookie: session-id=%s; Discard; domain=%s\r\n", s->guid,s->header->Host);
		printHeaderChunk(s->socket, "Set-Cookie: session-id=%s; Path=\"/\"; Discard\r\n", s->guid);	// Working but IE
		*/

		printHeaderChunk(s->socket, "Set-Cookie: session-id=%s; HttpOnly; Version=1; Path=/; Discard\r\n", s->guid); /* Working IE 11 , Opera 20 , Firefox, Chrome */
	}

	if ((s->create_cookie_ssl == 1) && (s->socket->use_ssl == 1 && (info->FileType == FILE_TYPE_HTML))) {
		/*
		printHeaderChunk(s->socket, "Set-Cookie: session-id-ssl=%s; Version=\"1\"; Path=\"/\"; Discard; Secure; HttpOnly; domain=%s\r\n",s->guid_ssl, s->header->Host);
		printHeaderChunk(s->socket, "Set-Cookie: session-id-ssl=%s; Path=\"/\"; Discard\r\n",s->guid_ssl, s->header->Host);
		*/
		printHeaderChunk(s->socket, "Set-Cookie: session-id-ssl=%s; HttpOnly; Version=1; Path=/; Discard; Secure\r\n",s->guid_ssl);

	}

	#else

	if ((s->create_cookie == 1) && (info->FileType == FILE_TYPE_HTML)) { /* Nur bei HTML Seiten Session cookies senden */
		//printHeaderChunk(s->socket, "Set-Cookie: session-id=%s; Version=1; Path=/; Discard; HttpOnly; domain=http://%s\r\n", s->guid,s->header->Host);
		printHeaderChunk(s->socket, "Set-Cookie: session-id=%s; Version=1; Path=/; Discard; HttpOnly\r\n", s->guid);
	}

	#endif
#endif
}





/*
 * 	p_lenght ist wichtig für die Template Engine
 */
int sendHeader(http_request* s, WebserverFileInfo *info, int p_lenght) {

	if( s->header->isHttp1_1 ){
		printHeaderChunk(s->socket, "HTTP/1.1 200 OK\r\n");
	}else{
		printHeaderChunk(s->socket, "HTTP/1.0 200 OK\r\n");
	}
	printHeaderChunk(s->socket, "Server: %s\r\n", "libCWebUI");

	addSessionCookies( s , info );

	printHeaderChunk(s->socket, "Accept-Ranges: bytes\r\n");
	
	if ( s->socket->use_output_compression == 0 ){
		printHeaderChunk(s->socket, "%s %d\r\n", "Content-Length:", p_lenght);
	}

	addCSPHeaderLines(s);

	addContentTypeLines(s, info);
	addCacheControlLines(s, info);
	addConnectionStatusLines(s->socket);

	if ( info->ForceDownload == 1){
		printHeaderChunk(s->socket, "Content-Description: File Transfer\r\n");
		/*
		printHeaderChunk(s->socket, "Content-Type: application/octet-stream\r\n");
		*/
		if ( 0 == strcmp("",(char*)info->ForceDownloadName)){
			printHeaderChunk(s->socket, "Content-Disposition: attachment; filename=\"%s\"\r\n",info->Url);
		}else{
			printHeaderChunk(s->socket, "Content-Disposition: attachment; filename=\"%s\"\r\n",info->ForceDownloadName);
		}
		printHeaderChunk(s->socket, "Content-Transfer-Encoding: binary\r\n");
		printHeaderChunk(s->socket, "Expires: 0\r\n");
		printHeaderChunk(s->socket, "Cache-Control: must-revalidate, post-check=0, pre-check=0\r\n");
		printHeaderChunk(s->socket, "Pragma: public\r\n");
	}
		
	if ( s->socket->use_output_compression == 0 ){
		printHeaderChunk(s->socket, "\r\n"); /* HTTP Header beenden */
	}
	return 1;
}

