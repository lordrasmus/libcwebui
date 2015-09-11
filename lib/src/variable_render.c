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





void VISIBLE_ATTR initRenderVariable(http_request* s){
	s->render_var_store = createVariableStore();
}

ws_variable* getRenderVariable(http_request* s,const char* name){
	return newVariable(s->render_var_store,name);
}

void VISIBLE_ATTR clearRenderVariables(http_request* s){
	deleteVariableStore(s->render_var_store);
	s->render_var_store = 0;
}


void setRenderVariable(http_request* s,const char* name,const char* text){
	ws_variable *var =  newVariable(s->render_var_store,name);
	setWSVariableString(var,text);
}


DEFINE_FUNCTION( dump_render_vars ){

	dumpStoreText( (http_request*)s, ((http_request*)s)->render_var_store, 0  );

}
