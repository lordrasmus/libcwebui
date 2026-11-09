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



int checkCookie(char *name,char *value,HttpRequestHeader *header){
	Cookie* cookie;
	int l;
	#ifdef _WEBSERVER_COOKIE_DEBUG_
	WebServerPrintf("Suche Cookie : %s ",name);
	#endif
	ws_list_iterator_start(&header->cookie_list);
	while( ( cookie = (Cookie*)ws_list_iterator_next(&header->cookie_list) ) ){
		if(0==strcmp( name, cookie->name )){
			l = strlen(cookie->value)+1;
			if(l > WEBSERVER_GUID_LENGTH){
				l = WEBSERVER_GUID_LENGTH;
			}
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

static void createCookie(HttpRequestHeader *header, char* name, unsigned int name_length, char* value, unsigned int value_length) {
	
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
	WebServerPrintf("Parsed Cookie ( %p ) Name <%s>  Value <%s> \n",cookie,cookie->name,cookie->value);
	#endif

	ws_list_append(&header->cookie_list,cookie, 0 );

}

enum CookieParseStates{
	PARSE_NAME,
	PARSE_VALUE
};

void parseCookies(char *line,int length,HttpRequestHeader *header) {
	int  i;

	enum CookieParseStates state = PARSE_NAME;

	/* The value starts after "Cookie:" and optional whitespace, and ends
	 * before trailing whitespace (RFC 9110 5.5, 5.6.3). It used to be taken
	 * 8 characters in, after "Cookie: " with exactly one space. */
	int skip = sizeof("Cookie:") - 1;
	while ( ( skip < length ) && ( ( line[skip] == ' ' ) || ( line[skip] == '\t' ) ) ){
		skip++;
	}
	while ( ( length > skip ) && ( ( line[length - 1] == ' ' ) || ( line[length - 1] == '\t' ) ) ){
		length--;
	}

	char* name_start = line + skip;
	char* name_end = line + skip;

	char* value_start = 0;
	char* value_end = 0;

	char bak1,bak2;

	line += skip;
	length -= skip;

#ifdef _WEBSERVER_DEBUG_
	WebServerPrintf("parseCookies : %s\n", line);
#endif


	for (i = 0; i < length; i++) {

		if ( ( state == PARSE_NAME ) && ( line[i] == ';' ) ){
			name_end = &line[i];

			if ( ( name_end - name_start ) > 0 ){

				bak1 = *name_end;
				*name_end = '\0';

				createCookie(header, name_start, name_end - name_start, 0, 0 );

				*name_end = bak1;
			}

			/* Skip semicolon and any whitespace */
			i++;
			while ( i < length && line[i] == ' ' ) {
				i++;
			}

			name_start = &line[i];
			i--; /* compensate for loop increment */

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

			/* Skip semicolon and any whitespace */
			i++;
			while ( i < length && line[i] == ' ' ) {
				i++;
			}

			name_start = &line[i];
			name_end = name_start;
			i--; /* compensate for loop increment */

			state = PARSE_NAME;
			continue;
		}

	}


	if ( state == PARSE_NAME ) {
		name_end = &line[i];

		/* Only create cookie if name is not empty */
		if ( ( name_end - name_start ) > 0 ){
			bak1 = *name_end;
			*name_end = '\0';

			createCookie(header, name_start, name_end - name_start, 0, 0 );

			*name_end = bak1;
		}
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

