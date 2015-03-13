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




void printHeaderInfo(HttpRequestHeader *header){
	Cookie* cookie;
	ws_variable *var;
	WebServerPrintf("\nURL : %s\n",header->url);

	WebServerPrintf("Parameter :\n");
	var = getFirstVariable(header->parameter_store);
	while(var != 0){		
		WebServerPrintf("  Name  : %s\n",var->name);
		WebServerPrintf("  Value : %s\n",var->val.value_string);
		var = getNextVariable(header->parameter_store);
	}
	
	WebServerPrintf("Cookies : \n");
	ws_list_iterator_start(&header->cookie_list);
	while( ( cookie = (Cookie*)ws_list_iterator_next(&header->cookie_list) ) ){
		WebServerPrintf("  Name  : %s\n",cookie->name);
		WebServerPrintf("  Value : %s\n",cookie->value);
	}
	

}





/*
 http://www.ietf.org/rfc/rfc2616.txt
 http://tools.ietf.org/html/rfc2965
 http://en.wikipedia.org/wiki/List_of_HTTP_headers
*/

