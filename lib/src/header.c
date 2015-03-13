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
	

/*	for(i=0;i<header->multiparametercount;i++){
		WebServerPrintf("  Name  : %s\n",header->multipartparameter[i]->name);
		if(header->multipartparameter[i]->contenttype==CONTENTTYP_TEXT)
			WebServerPrintf("  Value : %s\n\n",header->multipartparameter[i]->data);
		else
			WebServerPrintf("  Value : binary ( %d bytes )\n",header->multipartparameter[i]->lenght);
	}*/
}





// http://www.ietf.org/rfc/rfc2616.txt
// http://tools.ietf.org/html/rfc2965
// http://en.wikipedia.org/wiki/List_of_HTTP_headers

