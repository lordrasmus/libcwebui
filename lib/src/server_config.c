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


static ws_variable_store *config_v_store;

void initConfig(void) {
	config_v_store = createVariableStore();
	setConfigInt( "port",80);
	setConfigInt( "ssl_port",-1);
	setConfigInt( "use_csp",0);
	setConfigInt( "reload_py_modules",0);
	#ifdef WEBSERVER_SESSION_TIMEOUT
		setConfigInt( "session_timeout" , WEBSERVER_SESSION_TIMEOUT );
	#else
		setConfigInt( "session_timeout" , 300 );
	#endif
	//setConfigText( "server_ip","127.0.0.1");
}

void freeConfig(void){
	deleteVariableStore(config_v_store);
}

void setConfigText(const char* name, const char* text) {
	ws_variable *var;
	PlatformLockMutex(&config_v_store->lock);
	var = newVariable(config_v_store, name);
	setWSVariableString(var, text);
	PlatformUnlockMutex(&config_v_store->lock);
}

void setConfigInt(const char* name, const int value) {
	ws_variable *var;
	PlatformLockMutex(&config_v_store->lock);
	var = newVariable(config_v_store, name);
	setWSVariableInt(var, value);
	PlatformUnlockMutex(&config_v_store->lock);
}

char* getConfigText(const char* name) {
	ws_variable* var;
	var = getVariable(config_v_store, name);
	if (var != 0){
		return var->val.value_string;
	}
	return 0;
}

int getConfigInt(const char* name) {
	ws_variable* var = getVariable(config_v_store, name);
	if (var != 0){
		return getWSVariableInt(var);
	}
	return 0;
}
