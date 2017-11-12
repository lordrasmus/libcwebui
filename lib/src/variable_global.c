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

#include "webserver.h"

#ifdef DMALLOC
#include <dmalloc/dmalloc.h>
#endif


static ws_variable_store *global_v_store;

void initGlobalVariable() {
	global_v_store = createVariableStore();
}

void freeGlobalVariable() {
	ws_variable_store *tmp = global_v_store;
	if ( lockGlobals() == -1 ){
		LOG ( HANDLER_LOG,ERROR_LEVEL,0, "Fatal Error locking Globals exiting", "" );
		exit ( 1 );
	}
	global_v_store = 0;
	deleteVariableStore(tmp);
}

int lockGlobals() {
	if (global_v_store == 0) {
		return -1;
	}
	PlatformLockMutex(&global_v_store->lock);
	return 0;
}

int unlockGlobals() {
	if (global_v_store == 0) {
		return -1;
	}
	PlatformUnlockMutex(&global_v_store->lock);
	return 0;
}

ws_variable* setGlobalVariable(const char* name, const char* text) {
	ws_variable *var = newVariable(global_v_store, name);
	setWSVariableString(var, text);
	return var;
}

ws_variable* getGlobalVariable(const char* name) {
	ws_variable* ret = getVariable(global_v_store, name);
	if (ret == 0){
		ret = newVariable(global_v_store, name);
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

