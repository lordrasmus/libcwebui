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

#ifndef _VARIABLE_STORE_H_
#define _VARIABLE_STORE_H_

#include "list.h"
#include "variable.h"
#include "platform-defines.h"

struct ws_variable_store_s {
	list_t var_list;
	WS_MUTEX lock;
};

typedef struct ws_variable_store_s ws_variable_store;

#ifdef __cplusplus
extern "C" {
#endif

ws_variable_store* createVariableStore(void);
void lockStore(ws_variable_store* store);
void unlockStore(ws_variable_store* store);

void deleteVariableStore(ws_variable_store* store);
void clearVariables(ws_variable_store* store);
unsigned int getVariableStoreSize(ws_variable_store* store);

ws_variable* searchVariable(ws_variable_store* store, const char* name);
ws_variable* newVariable(ws_variable_store* store, const char* name);
ws_variable* getVariable(ws_variable_store* store, const char* name);
void addVariable(ws_variable_store* store, ws_variable* var);
void delVariable(ws_variable_store* store, const char* name);
ws_variable* refVariable(ws_variable_store* store, ws_variable* ref, const char* new_name);
void freeVariable(ws_variable_store* store, ws_variable* var);

ws_variable* getFirstVariable(ws_variable_store* store);
ws_variable* getNextVariable(ws_variable_store* store);
void stopIterateVariable(ws_variable_store* store);


#ifdef __cplusplus
}
#endif

#endif
