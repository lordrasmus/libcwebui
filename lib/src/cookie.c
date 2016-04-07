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

#include "stdafx.h"

#ifdef __GNUC__
	#include "webserver.h"
#endif

#ifdef DMALLOC
#include <dmalloc/dmalloc.h>
#endif




int checkCookie(char *name,char *value,HttpRequestHeader *header){
	Cookie* cookie;
	int l;
	#ifdef _WEBSERVER_COOKIE_DEBUG_
	WebServerPrintf("Suche Cookie : %s ",name);
	#endif
	ws_list_iterator_start(&header->cookie_list);
	while( ( cookie = (Cookie*)ws_list_iterator_next(&header->cookie_list) ) ){
		if(0==strcmp((char*)name,(char*)cookie->name)){
			l = strlen(cookie->value)+1;
			if(l > WEBSERVER_GUID_LENGTH)
			  l = WEBSERVER_GUID_LENGTH;
			memcpy(value,cookie->value,l);
			#ifdef _WEBSERVER_COOKIE_DEBUG_
			WebServerPrintf("-> gefunden Value <%s>\r\n",value);
			#endif
			ws_list_iterator_stop(&header->cookie_list);
			return 1;
		}
	}
	ws_list_iterator_stop(&header->cookie_list);

	#ifdef _WEBSERVER_COOKIE_DEBUG_
	WebServerPrintf("-> nicht gefunden\r\n");
	#endif
	return 0;
}

/*void copyCookieValue(char *line,HttpRequestHeader *header,int pos,int pos2){
	int pos3=-1,i,length;
	Cookie* cookie;
	for(i=pos;i<pos2;i++){
		if(line[i]=='='){
			pos3=i;
			break;
		}
	}

	if ( pos3 == -1 ){
		printf("invalid Cookie\n");
		return;
	}

	cookie = WebserverMallocCookie();

	if(pos3!=0){
		length = pos3-pos;
		cookie->name = (char*)WebserverMalloc(length+1);
		Webserver_strncpy(cookie->name,length+1,&line[pos],length);

		length = pos2-pos3-1;
		cookie->value = (char*)WebserverMalloc(length+1);
		Webserver_strncpy(cookie->value,length+1,&line[pos3+1],length);

	}else{
		length = pos2-pos+1;
		cookie->name = (char*)WebserverMalloc(length);
		Webserver_strncpy(cookie->name,length+1,&line[pos],length);

		cookie->value = (char*)WebserverMalloc(1);
		Webserver_strncpy(cookie->value,1,0,0);
	}

	#ifdef _WEBSERVER_COOKIE_DEBUG_
	WebServerPrintf("Parsed Cookie Name <%s>  Value <%s> \n",cookie->name,cookie->value);
	#endif

	ws_list_append(&header->cookie_list,cookie);
}*/

/*void parseCookies(char *line,int length,HttpRequestHeader *header){
	int pos,pos2;

	pos = 8;
	while(pos<length){
		pos2 = stringnfind(&line[pos],";",length-pos);
		if ( pos2 > 0 )
			pos2 += pos;
		else
			pos2 = length;

		if(pos2!=pos){
			copyCookieValue(line,header,pos,pos2);
		}else{
		  copyCookieValue(line,header,pos,length);
		}
		if(pos2==pos) break;
		pos=pos2+2;
	}

}*/

void createCookie(HttpRequestHeader *header, char* name, unsigned int name_length, char* value, unsigned int value_length) {
	ws_variable *var;
	int len;

	Cookie* cookie = WebserverMallocCookie();

	url_decode(name);
	len = strlen( name );
	cookie->name = (char*)WebserverMalloc(len+1);
	Webserver_strncpy(cookie->name,len+1,name,len );

	if (value != 0) {
		url_decode(value);
		len = strlen( value );
		cookie->value = (char*)WebserverMalloc(len+1);
		Webserver_strncpy(cookie->value,len+1,value,len);
	}else{
		cookie->value = (char*)WebserverMalloc(1);
		cookie->value[0] = '\0';
	}

	#ifdef _WEBSERVER_COOKIE_DEBUG_
	WebServerPrintf("Parsed Cookie Name <%s>  Value <%s> \n",cookie->name,cookie->value);
	#endif

	ws_list_append(&header->cookie_list,cookie);

}

enum CookieParseStates{
	PARSE_NAME,
	PARSE_VALUE
};

void parseCookies(char *line,int length,HttpRequestHeader *header) {
	int  i;

	enum CookieParseStates state = PARSE_NAME;

	char* name_start = line + 8;
	char* name_end = line + 8;

	char* value_start = 0;
	char* value_end = 0;

	char bak1,bak2;

	line += 8;

#ifdef _WEBSERVER_DEBUG_
	WebServerPrintf("parseCookies : %s\n", line);
#endif


	for (i = 0; i < length - 8; i++) {

		if ( ( state == PARSE_NAME ) && ( line[i] == ';' ) ){
			name_end = &line[i];

			if ( ( name_end - name_start ) > 0 ){

				bak1 = *name_end;
				*name_end = '\0';

				createCookie(header, name_start, name_end - name_start, 0, 0 );

				*name_end = bak1;
			}
			i++;

			name_start = &line[i+1];

			continue;
		}

		if ( ( state == PARSE_NAME ) && ( line[i] == '=' ) ){
			name_end = &line[i];
			value_start= &line[i+1];
			state = PARSE_VALUE;
			continue;
		}

		if ( ( state == PARSE_VALUE ) && ( line[i] == ';' ) ){
			value_end = &line[i];

			bak1 = *name_end;
			*name_end = '\0';

			bak2 = *value_end;
			*value_end = '\0';

			createCookie(header, name_start, name_end - name_start, value_start, value_end - value_start);

			*name_end = bak1;
			*value_end = bak2;

			value_start = 0;
			value_end = 0;

			i++;
			name_start = &line[i+1];
			name_end = name_start;

			state = PARSE_NAME;
			continue;
		}

	}


	if ( state == PARSE_NAME ) {
		name_end = &line[i];

		bak1 = *name_end;
		*name_end = '\0';

		createCookie(header, name_start, name_end - name_start, 0, 0 );

		*name_end = bak1;
	}

	if ( state == PARSE_VALUE ) {
		value_end = &line[i];

		bak1 = *name_end;
		*name_end = '\0';

		bak2 = *value_end;
		*value_end = '\0';

		createCookie(header, name_start, name_end - name_start, value_start, value_end - value_start);

		*name_end = bak1;
		*value_end = bak2;

	}

	/* WebServerPrintf("Anzahl Parameter : %d\n",header->paramtercount);
	 for(pos=0;pos<header->paramtercount;pos++){
	 WebServerPrintf("Parameter %d -> Name : %s \tValue : %s\n",pos,header->parameter[pos]->name,header->parameter[pos]->value);
	 }
	 */
}

