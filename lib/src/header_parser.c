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


#ifdef __GNUC__
#include <stdarg.h>
#include "webserver.h"
#endif

#ifdef DMALLOC
#include <dmalloc/dmalloc.h>
#endif


unsigned char toHex(unsigned char in) {
	if ((in <= '9') && (in >= '0')) {
		return (unsigned char)(in - '0');
	}

	if ((in >= 'a') && (in <= 'f')) {
		return (unsigned char)(in - 87);
	}

	if ((in >= 'A') && (in <= 'F')) {
		return (unsigned char)(in - 55 );
	}

	return 0;
}

void url_decode(char *line) {
	unsigned char hex;
	size_t i;
	size_t lenght;

	lenght = strlen((char*) line);
	for (i = 0; i < lenght; i++) {
		if (unlikely(line[i]=='%')) {
			hex = (unsigned char)(toHex(line[i + 1]) << 4);
			hex =  (unsigned char)( hex + toHex(line[i + 2]) );
			line[i] = hex; /*  hexcode des zeichens als char speichern und 2 zeichen loeschen */
			memcpy(&line[i + 1], &line[i + 3], lenght - i );
			lenght -= 2;
		}
		if (line[i] == '+'){ /* + durch whitespace ersetzen */
			line[i] = ' ';
		}
	}
}

void createParameter(HttpRequestHeader *header, char* name, unsigned int name_length, char* value, unsigned int value_length) {
	ws_variable *var;
	char back = name[name_length]; /* Buffer muss im original Zustand belassen werden */

	name[name_length] = '\0';
	var = newVariable(header->parameter_store, name);

	if (value != 0) {
		char back2 = value[value_length];
		value[value_length] = '\0';
		setWSVariableString(var, value);
		value[value_length] = back2;
	}

	name[name_length] = back;
}

void recieveParameterFromGet(char *line, HttpRequestHeader *header) {
	int http_pos, pos, pos2, i;
	char in_para_value;

	url_decode(line);

#ifdef _WEBSERVER_DEBUG_
	WebServerPrintf("recieveParameterFromGet : %s\n", line);
#endif

	http_pos = stringfind(line, "HTTP/");
	if (http_pos < 3) {
		return;
	}

	in_para_value = 0;

	pos = 0;
	pos2 = 0;

	for (i = 0; i < http_pos; i++) {
		if (line[i] == '&') {
			if (in_para_value == 0) {
				createParameter(header, &line[pos], i - pos, 0, 0);
			} else {
				createParameter(header, &line[pos], pos2 - pos - 1, &line[pos2], i - pos2);
			}

			pos = i + 1;
			in_para_value = 0;
		}
		if (line[i] == '=') {
			pos2 = i + 1;
			in_para_value = 1;
		}
	}

	if (in_para_value == 0) {
		createParameter(header, &line[pos], http_pos - 5 - pos, 0, 0);
	} else {
		createParameter(header, &line[pos], pos2 - pos - 1, &line[pos2], http_pos - 5 - pos2);
	}

	/* WebServerPrintf("Anzahl Parameter : %d\n",header->paramtercount);
	 for(pos=0;pos<header->paramtercount;pos++){
	 WebServerPrintf("Parameter %d -> Name : %s \tValue : %s\n",pos,header->parameter[pos]->name,header->parameter[pos]->value);
	 }
	 */
}

static void url_sanity_check( HttpRequestHeader *header ){
	int pos;

	header->error = 0;
	pos = stringfind(header->url, "<script>");
	if (pos > 0) {
		header->error = 1;
		return;
	} else {
		pos = stringfind(header->url, "</script>");
		if (pos > 0) {
			header->error = 1;
			return;
		}
	}

	pos = stringfind(header->url, "..");
	if (pos > 0) {
		header->error = 1;
		return;
	}
}

#define CHECK_HEADER_LINE(a,b)  h_len = strlen(a); \
if (!strncmp((char*)line,a,h_len)) \
{ \
	len = length - h_len; \
	if ( header->b != 0) WebserverFree(header->b); \
    header->b = (char*)WebserverMalloc( len + 1 ); \
    Webserver_strncpy((char*)header->b,len+1,(char*)&line[h_len],len); \
    return 0; \
}

#define CHECK_HEADER_LINE2(a,b)  h_len = strlen(a); \
if (!strncmp((char*)line2,a,h_len)) \
{ \
	len = stringfind(line2, "\r\n") - h_len - 1; \
	if ( header->b != 0) WebserverFree(header->b); \
    header->b = (char*)WebserverMalloc( len + 1 ); \
    Webserver_strncpy((char*)header->b,len+1,(char*)&line2[h_len],len); \
    line2 = &line2[h_len + len ]; length2 -= h_len + len ; continue ; \
}

int analyseFormDataLine(socket_info* sock, char *line, unsigned int length, HttpRequestHeader *header) {

	SIZE_TYPE h_len;
	unsigned long len;
	char *line2;

	unsigned int length2;

	length2 = length;
	line2 = line;

	while( length2 > 0 ){

		if ( 0 == strncmp(line2, "\r\n\r\n",4) ){
			/* Ende des Form Multipart Header Abschnitts */
			line2+=4;
			break;
		}

		/* POST Form Data Lines */
		CHECK_HEADER_LINE2("Content-Disposition: ",Content_Disposition)

		CHECK_HEADER_LINE2("Content-Type: ",Content_Type)

		length2--;
		line2++;

	}
	
#ifdef ENABLE_DEVEL_WARNINGS
	#warning "Noch genauer Testen"
#endif

	return line2 - line;
}

int analyseHeaderLine(socket_info* sock, char *line, unsigned int length, HttpRequestHeader *header) {
	unsigned long len;
	SIZE_TYPE h_len;
	int pos;
	unsigned int i;
	char* c_pos;
#ifdef _WEBSERVER_HEADER_DEBUG_
	LOG(HEADER_PARSER_LOG,NOTICE_LEVEL,0,"Header Line : %s",line);
#endif
	if (!strncmp((char*) line, "GET ", 4)) {
		header->method = HTTP_GET;
		pos = stringfind(&line[5], "?");
		if (pos == 0) /* keine parameter */
				{
			pos = stringfind(&line[5], " ");
			header->url = (char *) WebserverMalloc( pos + 1 );
			Webserver_strncpy((char*) header->url, pos + 1, (char*) &line[5], pos); /* -4 wegen dem GET am anfang */
		} else /* mit parametern */
		{
			header->url = (char *) WebserverMalloc( pos + 1 );
			Webserver_strncpy((char*) header->url, pos + 1, (char*) &line[5], pos); /* -4 wegen dem GET am anfang */
			recieveParameterFromGet(&line[5] + pos + 1, header);
		}

		url_sanity_check( header );

		if ( header->error == 1 ){
			return -1;
		}

		c_pos = strstr(line, "HTTP/1.1");
		if(c_pos != 0){
			sock->header->isHttp1_1 = 1;
		}
#if _WEBSERVER_CONNECTION_DEBUG_ > 3
		LOG(CONNECTION_LOG,NOTICE_LEVEL,sock->socket,"%s",line);
#endif

		return 0;
	}

	if(!strncmp(line,"POST ",5)){


#if _WEBSERVER_CONNECTION_DEBUG_ > 3
		LOG(CONNECTION_LOG,NOTICE_LEVEL,sock->socket,"%s",line);
#endif

		header->method=HTTP_POST;

		pos = stringfind(&line[6], "?");
		if (pos == 0) /* keine parameter */
		{
			pos = stringfind(&line[6], " ");
			header->url = (char *) WebserverMalloc( pos + 1 );
			Webserver_strncpy((char*) header->url, pos + 1, (char*) &line[6], pos); /* -4 wegen dem GET am anfang */
		} else /* mit parametern */
		{
			header->url = (char *) WebserverMalloc( pos + 1 );
			Webserver_strncpy((char*) header->url, pos + 1, (char*) &line[6], pos); /* -4 wegen dem GET am anfang */
			recieveParameterFromGet(&line[6] + pos + 1, header);
		}

		url_sanity_check( header );

		if ( header->error == 1 ){
			return -1;
		}

		c_pos = strstr(line, "HTTP/1.1");

		if(c_pos != 0){
			sock->header->isHttp1_1 = 1;
		}
#ifdef ENABLE_DEVEL_WARNINGS
		#warning "Noch genauer Testen"
#endif

		return 0;
	}

	if ( header->method == 0 ){
		header->error = 1;
		return -1;
	}

	/* noch nicht verarbeitete header lines */
	if (!strncmp((char*) line, "User-Agent: ", 12)) {
		return 0;
	}
	if (!strncmp((char*) line, "Accept: ", 8)) {
		return 0;
	}
	if (!strncmp((char*) line, "Accept-Language: ", 17)) {
		return 0;
	}
	if (!strncmp((char*) line, "Accept-Encoding: ", 17)) {
		return 0;
	}
	if (!strncmp((char*) line, "Accept-Charset: ", 16)) {
		return 0;
	}
	if (!strncmp((char*) line, "Keep-Alive: ", 12)) {
		return 0;
	}
	if (!strncmp((char*) line, "Referer: ", 9)) {
		return 0;
	}
	if (!strncmp((char*) line, "Cache-Control: ", 14)) {
		return 0;
	}

	if (!strncmp((char*) line, "Cookie: ", 8)) {
		parseCookies(line, length, header);
		return 0;
	}

	if (!strncmp((char*) line, "Content-Length: ", 15)) {
		header->contentlenght = atol((char*) &line[16]);
		return 0;
	}


	CHECK_HEADER_LINE("If-Modified-Since: ", If_Modified_Since) /* Wed, 12 Dec 2007 13:13:08 GMT */

	CHECK_HEADER_LINE("If-None-Match: ", etag)

	CHECK_HEADER_LINE("Upgrade: ", Upgrade)

	CHECK_HEADER_LINE("Connection: ", Connection)

	CHECK_HEADER_LINE("Host: ", Host)

	CHECK_HEADER_LINE("Origin: ", Origin)


#ifdef WEBSERVER_USE_WEBSOCKETS

	CHECK_HEADER_LINE("Sec-WebSocket-Key1: ",SecWebSocketKey1)

	CHECK_HEADER_LINE("Sec-WebSocket-Key2: ",SecWebSocketKey2)

	CHECK_HEADER_LINE("Sec-WebSocket-Key: ",SecWebSocketKey)

	CHECK_HEADER_LINE("Sec-WebSocket-Origin: ",SecWebSocketOrigin)

	CHECK_HEADER_LINE("Sec-WebSocket-Protocol: ",SecWebSocketProtocol)

	/* CHECK_HEADER_LINE("Sec-WebSocket-Version: ",SecWebSocketVersion) */

	h_len = strlen("Sec-WebSocket-Version: ");
	if (!strncmp((char*)line,"Sec-WebSocket-Version: ",h_len))
	{
		len = length - h_len;
		sscanf(&line[h_len],"%d",&header->SecWebSocketVersion);
		return 0;
	}
#endif

/*	V 8
Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==
Sec-WebSocket-Origin: http://example.com
Sec-WebSocket-Protocol: chat, superchat
Sec-WebSocket-Version

	V ?
Upgrade: WebSocket
Connection: Upgrade
Host: 192.168.1.50
Origin: http://192.168.1.50
*/
	/* mootools benutzt Content-type */
	if ( (!strncmp((char*) line, "Content-Type: ", 14)) || (!strncmp((char*) line, "Content-type: ", 14)) ){
		if(stringfind(&line[14],"multipart/form-data") > 0){
			int i2=stringfind(line,"boundary=");
			header->contenttype=MULTIPART_FORM_DATA;

			i2++;

			for(i=i2;i<length;i++)
				if(line[i]=='\r')
					break;

			header->boundary=(char*)WebserverMalloc( ( i - i2 )  + 3 ); /* + 2 für -- , +1 \0 */
			strncpy(header->boundary,"--",2);		/* nach http://www.w3.org/Protocols/rfc1341/7_2_Multipart.html */
			strncpy(header->boundary+2,&line[i2],i-i2);	/* kommt vor die boundary -- */
			header->boundary[i-i2+2]='\0';
#ifdef ENABLE_DEVEL_WARNINGS
			#warning Noch mehr Fehlerprüfungen
#endif
			return 0;
		}

		if(!strncmp(&line[14],"application/x-www-form-urlencoded",strlen("application/x-www-form-urlencoded"))){
			header->contenttype=APPLICATION_X_WWW_FORM_URLENCODED;
			return 0;
		}
	}

	return 0;

}

/*
 *
 *	Return Values
 *
 *		>0  = Header nicht zuende weiter Daten lesen
 * 		 0  = Fehler beim Parsen
 *		-3  = Header zuende aber noch weitere daten vorhanden
 * 		-2  = Header ist zuende und keine weiteren Daten vorhanden
 *
 *
 *
 */

int ParseHeader(socket_info* sock, HttpRequestHeader* header, char* buffer, unsigned int length, unsigned int* bytes_parsed) {
	unsigned int i = 0;
	unsigned int last_line_end = 0;
	char* pos = buffer;
	char back;
#ifdef WEBSERVER_USE_WEBSOCKETS
	int diff,ret;
#endif
	unsigned int line_length;


	if (length < 2) return length;

	if (length < 4) {
		if ((buffer[0] == '\r') && (buffer[1] == '\n')) {
#ifdef WEBSERVER_USE_WEBSOCKETS
			ret = checkIskWebsocketConnection(sock,header);
			if( ret > 1) {
				diff = length -i;
				if( diff == 9) {
					memcpy(header->WebSocketKey3,&buffer[i+1],8);
					return -2;
				} else {
					/* TODO: Behandlung wenn der Teil nach dem Websocket header nicht vollstaendig ist */
					printf("Fehlt noch \n");
				}
			}
#endif
			return -2;
		}
	}
	for (i = 3; i < length; i++) {
		if ((buffer[i - 3] == '\r') && (buffer[i - 2] == '\n') && (buffer[i - 1] == '\r') && (buffer[i - 0] == '\n')) { /* Ende des HTTP Headers ( \r\n\r\n ) */

#ifdef WEBSERVER_USE_WEBSOCKETS
			ret = checkIskWebsocketConnection(sock,header);
			if( ret > 1) {
				if ( header->SecWebSocketVersion < 13 ) {
					printf("Warning SecWebSocketVersion < 13\n");
					diff = length -i;
					if( diff == 9) {
						memcpy(header->WebSocketKey3,&buffer[i+1],8);
					} else {
						printf("Fehlt noch \n"); 
						/* TODO: Behandlung wenn der Teil nach dem Websocket header nicht vollstaendig ist */
					}
				}
			}
#endif

			*bytes_parsed = i;
			/* es ist ein weiterer header im datenstrom gewesen */
			if ( (i + 1 ) < length) return -3;
			return -2;

		}

		if ((buffer[i] == '\n') && (buffer[i - 1] == '\r')) /* Am Ende der Zeile steht bei http immer \r\n */
				{
			back = buffer[i - 1];
			buffer[i - 1] = '\0';
			line_length = &buffer[i - 1] - pos;
			if ( analyseHeaderLine(sock, pos, line_length, header) < 0 ){
				return -4;
			}
			pos = &buffer[i + 1];
			last_line_end = i + 1;
			buffer[i - 1] = back;
		}

	}
	*bytes_parsed = last_line_end;
	return last_line_end;
}

void clearHeader(http_request *s) {
	s->header->url = 0;
	s->header->method = 0;
	s->header->gzip = 0;
	s->header->deflate = 0;
	s->header->contentlenght = 0;
	s->header->contenttype = 0;
	clearVariables(s->header->parameter_store);
	ws_list_clear(&s->header->cookie_list);
}

ws_variable* getParameter(http_request* s, const char* name) {
	return getVariable(s->header->parameter_store, name);
}

bool checkHeaderComplete(HttpRequestHeader* header) {
	if (header->url == 0) return false;

	return true;
}

