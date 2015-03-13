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


DEFINE_FUNCTION_INT( dump_render_vars ){

	dumpStoreText( s, s->render_var_store, 0  );

}
