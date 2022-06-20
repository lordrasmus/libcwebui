/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/



#ifndef _VARIABLE_STORE_H_
#define _VARIABLE_STORE_H_

#include "platform-defines.h"
#include "variable.h"


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
ws_variable* newVariable(ws_variable_store* store, const char* name, uint32_t flags );
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
