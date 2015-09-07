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

#include "helper.h"

#ifndef _VARIABLE_H_
#define _VARIABLE_H_

typedef enum
{
    VAR_TYPE_EMPTY,
    VAR_TYPE_STRING,
    VAR_TYPE_INT,
    VAR_TYPE_ULONG,
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

ALL_EXTRA int 			cmpVariableStoreName(void* var,char* name) ALL_ATTR;

/**********************************************************
*                                                         *
*               Verwaltungsfunktionen                     *
*                                                         *
**********************************************************/

ws_variable* 	newWSVariable(const char* name) ALL_ATTR;
ws_variable*    newWSArray(const char* name) ALL_ATTR;
void 			freeWSVariable(ws_variable* var) ALL_ATTR;
void 			freeWSVariableValue(ws_variable* var) ALL_ATTR;
SIZE_TYPE		getWSVariableSize(ws_variable* var) ALL_ATTR;



/**********************************************************
*                                                         *
*               Zugriffsfunktionen                        *
*                                                         *
**********************************************************/

ALL_EXTRA void 			setWSVariableString(ws_variable* var, const char* text) ALL_ATTR;
ALL_EXTRA int 			getWSVariableString(ws_variable* var, char* buffer,unsigned int buffer_length) ALL_ATTR;

ALL_EXTRA void          setWSVariableULong(ws_variable* var, uint64_t value) ALL_ATTR;
ALL_EXTRA void 			setWSVariableInt(ws_variable* var, int value) ALL_ATTR;
ALL_EXTRA uint64_t   	getWSVariableULong(ws_variable* var) ALL_ATTR;
ALL_EXTRA int 			getWSVariableInt(ws_variable* var) ALL_ATTR;

ALL_EXTRA void			setWSVariableRef(ws_variable* var,ws_variable* ref) ALL_ATTR;

ALL_EXTRA void 			setWSVariableCustomData(ws_variable* var, var_free_handler handle, void* data ) ALL_ATTR;


/**********************************************************
*                                                         *
*               Arrayfunktionen                           *
*                                                         *
**********************************************************/

ALL_EXTRA void 			setWSVariableArray(ws_variable* var) ALL_ATTR;
ALL_EXTRA ws_variable*	getWSVariableArray(ws_variable* var, const char* name) ALL_ATTR;
ALL_EXTRA ws_variable*	addWSVariableArray(ws_variable* var, const char* name) ALL_ATTR;
ALL_EXTRA void			delWSVariableArray(ws_variable* var, const char* name) ALL_ATTR;
ALL_EXTRA ws_variable*	refWSVariableArray(ws_variable* var, ws_variable* ref) ALL_ATTR;

ALL_EXTRA ws_variable*	getWSVariableArrayFirst(ws_variable* var) ALL_ATTR;
ALL_EXTRA ws_variable*	getWSVariableArrayNext(ws_variable* var) ALL_ATTR;
ALL_EXTRA void 			stopWSVariableArrayIterate(ws_variable* var) ALL_ATTR;

ALL_EXTRA ws_variable*	getWSVariableArrayIndex(ws_variable* var, unsigned int index) ALL_ATTR;
ALL_EXTRA ws_variable*	addWSVariableArrayIndex(ws_variable* var, unsigned int index) ALL_ATTR;



#ifdef __cplusplus
}
#endif


#endif
