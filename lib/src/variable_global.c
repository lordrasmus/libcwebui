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


static ws_variable_store *global_v_store;

void initGlobalVariable( void ) {
	global_v_store = createVariableStore();
}

void freeGlobalVariable( void ) {
	ws_variable_store *tmp = global_v_store;
	if ( lockGlobals() == -1 ){
		LOG ( HANDLER_LOG,ERROR_LEVEL,0, "%s","Fatal Error locking Globals exiting" );
		exit ( 1 );
	}
	global_v_store = 0;
	deleteVariableStore(tmp);
}

int lockGlobals( void ) {
	if (global_v_store == 0) {
		return -1;
	}
	PlatformLockMutex(&global_v_store->lock);
	return 0;
}

int unlockGlobals( void ) {
	if (global_v_store == 0) {
		return -1;
	}
	PlatformUnlockMutex(&global_v_store->lock);
	return 0;
}

ws_variable* setGlobalVariable(const char* name, const char* text) {
	ws_variable *var = newVariable(global_v_store, name, 0 );
	setWSVariableString(var, text);
	return var;
}

ws_variable* getGlobalVariable(const char* name) {
	ws_variable* ret = getVariable(global_v_store, name);
	if (ret == 0){
		ret = newVariable(global_v_store, name, 0 );
	}
	return ret;
}

ws_variable* getExistentGlobalVariable(const char* name) {
	return getVariable(global_v_store, name);
}

void dumpGlobals(http_request* s) {
	ws_variable *var;
	ws_variable *var_array;
	char buffer[200];

	var_array = getRenderVariable(s, "globals_dump");
	setWSVariableArray(var_array);

	var = getFirstVariable(global_v_store);
	while (var != 0) {
		refWSVariableArray(var_array, var);
		getWSVariableString(var, buffer, 200);
		var = getNextVariable(global_v_store);
	}

}

