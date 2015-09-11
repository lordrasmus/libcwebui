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

void copyCookieValue(char *line,HttpRequestHeader *header,int pos,int pos2){
	int pos3=0,i,length;
	Cookie* cookie;
	for(i=pos;i<pos2;i++){
		if(line[i]=='='){
			pos3=i;
			break;
		}
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
}

void parseCookies(char *line,int length,HttpRequestHeader *header){
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
	
}

