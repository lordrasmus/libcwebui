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

#include <stdarg.h>
#include "webserver.h"

#ifdef DMALLOC
#include <dmalloc/dmalloc.h>
#endif


// https://www.html5rocks.com/static/images/cors_server_flowchart.png

static cors_handler cors_handle_func = 0;

void setCORS_Handler( cors_handler handler ){
	cors_handle_func = handler;
	LOG(HEADER_PARSER_LOG,NOTICE_LEVEL,0,"CORS Handler set","");
}


int checkCORS( CORS_HEADER_TYPES type, socket_info* socket ){

	if ( cors_handle_func == 0 ) return COND_FALSE;

	cors_infos info;
	info.type = type;

	if ( socket->header->Origin != 0 )
		info.origin = socket->header->Origin;
	else
		info.origin = "";

	if ( socket->header->method == HTTP_OPTIONS )
		info.method = "OPTIONS";

	if ( socket->header->method == HTTP_GET )
		info.method = "GET";

	if ( socket->header->method == HTTP_POST )
		info.method = "POST";

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
	if (sock->header->SecWebSocketVersion < 7) {
		printWebsocketChunk(sock, "HTTP/1.1 101 Web Socket Protocol Handshake\r\n");
		printWebsocketChunk(sock, "Upgrade: WebSocket\r\n");
		printWebsocketChunk(sock, "Connection: Upgrade\r\n");
		if (sock->header->SecWebSocketKey1 != 0) { /* neuers websocket handshake protokol */
			printWebsocketChunk(sock, "sec-websocket-origin: %s\r\n", sock->header->Origin);
			printWebsocketChunk(sock, "sec-websocket-location: ws://%s/%s\r\n", sock->header->Host, sock->header->url);
		} else {
			printWebsocketChunk(sock, "WebSocket-Origin: %s\r\n", sock->header->Origin);
			printWebsocketChunk(sock, "WebSocket-Location: ws://%s/%s\r\n", sock->header->Host, sock->header->url);
		}
		if (sock->header->SecWebSocketKey1 != 0) {
			sendWebsocketChunk(sock, sock->header->WebSocketOutHash, 16);
		}
	} else {
		printWebsocketChunk(sock, "HTTP/1.1 101 Switching Protocols\r\n");
		printWebsocketChunk(sock, "Upgrade: websocket\r\n");
		printWebsocketChunk(sock, "Connection: Upgrade\r\n");
		printWebsocketChunk(sock, "Sec-WebSocket-Accept: %s\r\n", sock->header->WebSocketOutHash);
	}
	printWebsocketChunk(sock, "\r\n");
	return 0;
}

#endif


void addConnectionStatusLines(socket_info* socket) {

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

int sendPreflightAllowed(socket_info *socket) {

	printHeaderChunk( socket, "HTTP/1.1 204 No Content\r\n");
	printHeaderChunk( socket, "Server: %s\r\n", "libCWebUI");


	if ( ( socket->header->Access_Control_Request_Method != 0 ) && ( COND_TRUE == checkCORS( CORS_ALLOW_METHODS, socket ) ) ){
		printHeaderChunk( socket, "Access-Control-Allow-Methods: %s\r\n",socket->header->Access_Control_Request_Method);
	}

	if ( ( socket->header->Access_Control_Request_Headers != 0 ) && ( COND_TRUE == checkCORS( CORS_ALLOW_HEADERS, socket ) ) ){
		printHeaderChunk( socket, "Access-Control-Allow-Headers: %s\r\n",socket->header->Access_Control_Request_Headers);
	}

	// Access-Control-Max-Age

	if ( ( socket->header->Origin != 0 ) && ( COND_TRUE == checkCORS( CORS_ALLOW_ORIGIN, socket ) ) ){
		printHeaderChunk( socket, "Access-Control-Allow-Origin: %s\r\n",socket->header->Origin);
	}

	if ( COND_TRUE == checkCORS( CORS_ALLOW_CREDENTIALS, socket ) ){
		printHeaderChunk( socket, "Access-Control-Allow-Credentials: true\r\n");
	}

	printHeaderChunk( socket, "\r\n"); /* HTTP Header beenden */

	return 1;
}

void addCacheControlLines(http_request* s, WebserverFileInfo *info) {
	/* Cache-Control: no-cache, no-store, must-revalidate, pre-check=0, post-check=0 */
#ifndef WEBSERVER_DISABLE_CACHE
	if ( ( info->FileType != FILE_TYPE_HTML ) && ( info->TemplateFile == 0   ) ) {
		printHeaderChunk ( s->socket,"Cache-Control: max-age=%d, public\r\n",MAX_CACHE_AGE ); /* sekunden bis refresh */
		/* printHeaderChunk ( s->socket,"Expires: Thu, 15 Apr 2060 20:00:00 GMT\r\n"); */

		if ( info->etag != 0 )
			printHeaderChunk ( s->socket,"ETag: %s\r\n",info->etag );

		if ( info->lastmodified != 0 )
			printHeaderChunk ( s->socket,"Last-Modified: %s\r\n",info->lastmodified );

	}
#endif
}

int sendHeaderNotModified(http_request* s, WebserverFileInfo *info) {
	printHeaderChunk(s->socket, "HTTP/1.1 304 Not Modified\r\n");
	printHeaderChunk(s->socket, "Server: %s\r\n", "libCWebUI");
	addCacheControlLines(s, info);
	addConnectionStatusLines(s->socket);
	printHeaderChunk(s->socket, "\r\n");
	return 0;
}

void sendHeaderNotFound(http_request* s, int p_lenght) {
	printHeaderChunk(s->socket, "HTTP/1.1 404 Not Found\r\n");
	printHeaderChunk(s->socket, "Server: %s\r\n", "libCWebUI");
	printHeaderChunk(s->socket, "Content-Length: %d\r\n", p_lenght);
	printHeaderChunk(s->socket, "Content-Type: text/html\r\n");
	addConnectionStatusLines(s->socket);
	printHeaderChunk(s->socket, "\r\n");
}

void sendHeaderError(socket_info* socket, char* ErrorMessage, int p_lenght) {
	printHeaderChunk(socket, "HTTP/1.1 %s\r\n", ErrorMessage);
	printHeaderChunk(socket, "Server: %s\r\n", "libCWebUI");
	printHeaderChunk(socket, "Content-Length: %d\r\n", p_lenght);
	printHeaderChunk(socket, "Content-Type: text/html\r\n");
	addConnectionStatusLines(socket);
	printHeaderChunk(socket, "\r\n");
}

void addContentTypeLines(http_request *s, WebserverFileInfo *info) {

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

void addFirePHPHeaderLines(http_request* s) {
#ifdef WEBSERVER_USE_SSL
	int log_lines = 0, i;
	int start_offset = 0, len = 0;
	FireLogger *fl;
	static char buffer[10000];
	static char buffer2[10000];
	int input_offset = 0;

	if (ws_list_empty(&s->socket->firephplogs) == 0) {
		input_offset += sprintf(&buffer2[input_offset], "{\"logs\":[");
		ws_list_iterator_start(&s->socket->firephplogs);
		while ( ( fl = (FireLogger*)ws_list_iterator_next(&s->socket->firephplogs) ) ) {
			input_offset += sprintf(&buffer2[input_offset], "{\"name\":\"miniWebserver\",\"args\":[],\"level\":\"debug\"");
			input_offset += sprintf(&buffer2[input_offset], ",\"timestamp\":1314144541.89,\"order\":0,\"time\":\"00:09:01.890\"");
			input_offset += sprintf(&buffer2[input_offset], ",\"template\":\"%s\",\"message\":\"%s\"", fl->text, fl->text);
			input_offset += sprintf(&buffer2[input_offset], ",\"style\":\"background-color: #767ab6\"");
			input_offset += sprintf(&buffer2[input_offset], ",\"pathname\":\"%s\",\"lineno\":%d},", fl->file, fl->line);
			WebserverFree(fl->text);
			WebserverFree(fl);
		}
		ws_list_iterator_stop(&s->socket->firephplogs);
		ws_list_clear(&s->socket->firephplogs);
		input_offset--;
		sprintf(&buffer2[input_offset], "]}");

		WebserverBase64Encode((const unsigned char *) buffer2, strlen(buffer2), (unsigned char *) buffer, 10000);

		len = strlen(buffer);
		for (i = 0; i < len; i++) {
			if (buffer[i] == '\n') {
				buffer[i] = '\0';
				printHeaderChunk(s->socket, "FireLogger-fe618963-%d: %s\r\n", log_lines++, &buffer[start_offset]);
				start_offset = i + 1;
			}
		}
		printHeaderChunk(s->socket, "FireLogger-fe618963-%d: %s\r\n", log_lines++, &buffer[start_offset]);
	}
#endif

	/*
	 * 	FireLogger-7ff46714-0	eyJsb2dzIjpbeyJuYW1lIjoicGhwIiwiYXJncyI6W10sImxldmVsIjoiZGVidWciLCJ0aW1lc3Rh
	 FireLogger-7ff46714-1	bXAiOjEzMTQxNDQ1NDEuODksIm9yZGVyIjowLCJ0aW1lIjoiMDA6MDk6MDEuODkwIiwidGVtcGxh
	 FireLogger-7ff46714-2	dGUiOiJIZWxsbyB3b3JsZCEiLCJtZXNzYWdlIjoiSGVsbG8gd29ybGQhIiwic3R5bGUiOiJiYWNr
	 FireLogger-7ff46714-3	Z3JvdW5kLWNvbG9yOiAjNzY3YWI2IiwicGF0aG5hbWUiOiJcL3Zhclwvd3d3XC90ZXN0XC9pbmRl
	 FireLogger-7ff46714-4	eC5waHAiLCJsaW5lbm8iOjN9XX0=
	 */

	/*len = snprintf ( buff, 450,"[{\"Type\":\"LOG\",\"File\":\"%s\",\"Line\":%d},\"",filename,fileline );
	len+= snprintf ( buff+len,450-len,"\"]" );
	len = snprintf(buff,200,"[{\"Type\":\"LOG\",\"File\":\"%s\",\"Line\":%d},\"%s\"]",filename,fileline,text);
	 http://www.firephp.org/Wiki/Reference/Protocol
	 http://wildfirehq.org/
	*/
	/*printHeaderChunk ( s->socket,"FireLogger: http://meta.wildfirehq.org/Protocol/JsonStream/0.2\r\n" );
	 printHeaderChunk ( s->socket,"X-Wf-Protocol-1: http://meta.wildfirehq.org/Protocol/JsonStream/0.2\r\n" );
	 printHeaderChunk ( s->socket,"X-Wf-1-Plugin-1: http://meta.firephp.org/Wildfire/Plugin/FirePHP/Library-FirePHPCore/0.3\r\n" );
	 printHeaderChunk ( s->socket,"X-Wf-1-Structure-1: http://meta.firephp.org/Wildfire/Structure/FirePHP/FirebugConsole/0.1\r\n" );

	 liste = getFirstListNode ( s->header->firephplogs );
	 while ( liste != 0 ) {
	 printHeaderChunk ( s->socket,"X-Wf-1-1-1-%d: %d|%s|\r\n",log_lines++, ( int ) strlen ( ( char* ) liste->value ), ( char* ) liste->value );
	 liste = getNextListNode ( liste );
	 }*/
}

void addCSPHeaderLines(http_request* s){

	char buff[1000];
	int offset = 0;

	if ( getConfigInt( "use_csp") == 0 )
		return;

	/* ; style-src 'self' ; img-src 'self' ; script-src 'self' */

#ifdef WEBSERVER_USE_SSL

	offset += snprintf(&buff[offset],1000-offset,"default-src http://%s https://%s; ",s->header->Host ,s->header->Host);
	/*offset += snprintf(&buff[offset],1000-offset,"script-src 'self' 'unsafe-eval' http://%s https://%s; ",s->header->Host ,s->header->Host);*/
	offset += snprintf(&buff[offset],1000-offset,"script-src http://%s https://%s; ",s->header->Host ,s->header->Host);
	offset += snprintf(&buff[offset],1000-offset,"style-src http://%s https://%s; ",s->header->Host ,s->header->Host);
	          snprintf(&buff[offset],1000-offset,"connect-src ws://%s http://%s wss://%s https://%s ; ",s->header->Host ,s->header->Host,s->header->Host ,s->header->Host );

#else

	offset += snprintf(&buff[offset],1000-offset,"default-src http://%s; ",s->header->Host );
	offset += snprintf(&buff[offset],1000-offset,"script-src http://%s; ",s->header->Host );
	offset += snprintf(&buff[offset],1000-offset,"style-src http://%s; ",s->header->Host );
	          snprintf(&buff[offset],1000-offset,"connect-src ws://%s http://%s; ",s->header->Host ,s->header->Host );

#endif


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

void addSessionCookies(http_request* s,WebserverFileInfo *info){
#ifdef WEBSERVER_USE_SESSIONS

	#ifdef WEBSERVER_USE_SSL

	if ((s->create_cookie == 1) && (info->FileType == FILE_TYPE_HTML)) { /* Nur bei HTML Seiten Session cookies senden */
		/*
		printHeaderChunk(s->socket, "Set-Cookie: session-id=%s; Version=\"1\"; Path=\"/\"; Discard; HttpOnly; domain=%s\r\n", s->guid,s->header->Host);
		printHeaderChunk(s->socket, "Set-Cookie: session-id=%s; Discard; domain=%s\r\n", s->guid,s->header->Host);
		printHeaderChunk(s->socket, "Set-Cookie: session-id=%s; Path=\"/\"; Discard\r\n", s->guid);	// Working but IE
		*/

		printHeaderChunk(s->socket, "Set-Cookie: session-id=%s; HttpOnly; Discard\r\n", s->guid); /* Working IE 11 , Opera 20 , Firefox, Chrome */
	}

	if ((s->create_cookie_ssl == 1) && (s->socket->use_ssl == 1 && (info->FileType == FILE_TYPE_HTML))) {
		/*
		printHeaderChunk(s->socket, "Set-Cookie: session-id-ssl=%s; Version=\"1\"; Path=\"/\"; Discard; Secure; HttpOnly; domain=%s\r\n",s->guid_ssl, s->header->Host);
		printHeaderChunk(s->socket, "Set-Cookie: session-id-ssl=%s; Path=\"/\"; Discard\r\n",s->guid_ssl, s->header->Host);
		*/
		printHeaderChunk(s->socket, "Set-Cookie: session-id-ssl=%s; HttpOnly; Discard; Secure\r\n",s->guid_ssl);

	}

	#else

	if ((s->create_cookie == 1) && (info->FileType == FILE_TYPE_HTML)) { /* Nur bei HTML Seiten Session cookies senden */
		printHeaderChunk(s->socket, "Set-Cookie: session-id=%s; Version=\"1\"; Path=\"/\"; Discard; HttpOnly; domain=http://%s\r\n", s->guid,s->header->Host);
	}

	#endif
#endif
}





/*
 * 	p_lenght ist wichtig für die Template Engine
 */
int sendHeader(http_request* s, WebserverFileInfo *info, int p_lenght) {


	printHeaderChunk(s->socket, "HTTP/1.1 200 OK\r\n");
	printHeaderChunk(s->socket, "Server: %s\r\n", "libCWebUI");

	addSessionCookies( s , info );

	printHeaderChunk(s->socket, "Accept-Ranges: bytes\r\n");
	
	if ( s->socket->use_output_compression == 0 )
		printHeaderChunk(s->socket, "%s %d\r\n", "Content-Length:", p_lenght);

	addCSPHeaderLines(s);

	addContentTypeLines(s, info);
	addCacheControlLines(s, info);
	addConnectionStatusLines(s->socket);

	if (info->FileType == FILE_TYPE_HTML) {
		addFirePHPHeaderLines(s);
	}

	if ( info->ForceDownload == 1){
		printHeaderChunk(s->socket, "Content-Description: File Transfer\r\n");
		/*
		printHeaderChunk(s->socket, "Content-Type: application/octet-stream\r\n");
		*/
		if ( 0 == strcmp("",(char*)info->ForceDownloadName)){
			printHeaderChunk(s->socket, "Content-Disposition: attachment; filename=%s\r\n",info->Url);
		}else{
			printHeaderChunk(s->socket, "Content-Disposition: attachment; filename=%s\r\n",info->ForceDownloadName);
		}
		printHeaderChunk(s->socket, "Content-Transfer-Encoding: binary\r\n");
		printHeaderChunk(s->socket, "Expires: 0\r\n");
		printHeaderChunk(s->socket, "Cache-Control: must-revalidate, post-check=0, pre-check=0\r\n");
		printHeaderChunk(s->socket, "Pragma: public\r\n");
	}
		
	if ( s->socket->use_output_compression == 0 )
		printHeaderChunk(s->socket, "\r\n"); /* HTTP Header beenden */
	return 1;
}

