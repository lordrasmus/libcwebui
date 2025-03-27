/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/


#include "helper.h"
#include "platform-defines.h"


#ifndef _VARIABLE_H_
#define _VARIABLE_H_

typedef enum
{
    VAR_TYPE_EMPTY,
    VAR_TYPE_STRING,
    VAR_TYPE_INT,
    VAR_TYPE_ULONG,
    VAR_TYPE_LONG,
    VAR_TYPE_ARRAY,
	VAR_TYPE_REF,
	VAR_TYPE_CUSTOM_DATA
} VAR_TYPES;

struct ws_variable_store_s;
struct ws_variable_s;

typedef void ( *var_free_handler ) ( void* ptr);

struct ws_variable_s
{
    VAR_TYPES type;
    char* name;
    union
    {
        void* value_p;
        char* value_string;
        int   value_int;
        uint64_t  value_uint64_t;
        int64_t  value_int64_t;
		struct ws_variable_store_s* value_array;
		struct ws_variable_s* value_ref;
	}val;
	union
	{
		SIZE_TYPE  str_len;
		var_free_handler handle;
	}extra;

	SIZE_TYPE  name_len;

};

typedef struct ws_variable_s ws_variable;


#ifdef __cplusplus
extern "C" {
#endif

int 			cmpVariableStoreName(void* var,char* name);

/**********************************************************
*                                                         *
*               Verwaltungsfunktionen                     *
*                                                         *
**********************************************************/

ws_variable* 	newWSVariable(const char* name);
ws_variable*    newWSArray(const char* name);
void 			freeWSVariable(ws_variable* var);
void 			freeWSVariableValue(ws_variable* var);
SIZE_TYPE		getWSVariableSize(ws_variable* var);



/**********************************************************
*                                                         *
*               Zugriffsfunktionen                        *
*                                                         *
**********************************************************/

void		setWSVariableString(ws_variable* var, const char* text);
int 		getWSVariableString(ws_variable* var, char* buffer,unsigned int buffer_length);

void		setWSVariableULong(ws_variable* var, uint64_t value);
uint64_t   	getWSVariableULong(ws_variable* var);

void		setWSVariableLong(ws_variable* var, int64_t value);
int64_t   	getWSVariableLong(ws_variable* var);

void		setWSVariableInt(ws_variable* var, int value);
int			getWSVariableInt(ws_variable* var);


void		setWSVariableRef(ws_variable* var,ws_variable* ref);

void 		setWSVariableCustomData(ws_variable* var, var_free_handler handle, void* data );


/**********************************************************
*                                                         *
*               Arrayfunktionen                           *
*                                                         *
**********************************************************/

void 			setWSVariableArray(ws_variable* var);
ws_variable*	getWSVariableArray(ws_variable* var, const char* name);
ws_variable*	addWSVariableArray(ws_variable* var, const char* name, uint32_t flags );
void			delWSVariableArray(ws_variable* var, const char* name);
ws_variable*	refWSVariableArray(ws_variable* var, ws_variable* ref);

ws_variable*	getWSVariableArrayFirst(ws_variable* var);
ws_variable*	getWSVariableArrayNext(ws_variable* var);
void 			stopWSVariableArrayIterate(ws_variable* var);

ws_variable*	getWSVariableArrayIndex(ws_variable* var, unsigned int index);
ws_variable*	addWSVariableArrayIndex(ws_variable* var, unsigned int index);



#ifdef __cplusplus
}
#endif


#endif
