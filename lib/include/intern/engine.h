/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/


#ifndef _ENGINE_H_
#define _ENGINE_H_

#include "../webserver_api_functions_macros.h"
#include "webserver.h"


typedef enum {
  IF_TAG_GROUP,
  LOOP_TAG_GROUP
}tag_groups;

typedef enum {
  IF_TAG,
  ELSE_TAG,
  ENDIF_TAG,

  LOOP_TAG,
  ENDLOOP_TAG
}TAG_IDS;




#ifdef __cplusplus
extern "C" {
#endif

void init_extension_api(void);
void free_extension_api(void);

/*void engine_start(http_request *s);
void engine_end(http_request *s);*/

int getNextTag ( const char* pagedata,const int datalength, tag_groups search_tag_group,TAG_IDS *tag );
int find_tag_end_pos(const char *pagedata, int datalenght,const char *start_tag,const char *end_tag);

void getFunction(const unsigned char *para,int *function,int *id);

int processHTML(http_request* s,const char* prefix,const char *pagename,const char *pagedata, int datalenght);

void register_function(const char* name,user_function func,const char* file,int line);
#ifdef WEBSERVER_USE_PYTHON
void register_py_function(const char* name, PyObject * py_func, const char* file, int line);
#endif
void register_condition(const char* name,user_condition f,const char* file,int line);
void register_function_websocket_handler(const char* url,websocket_handler f,const char* file,int line);
int loadPlugin(const char* name,const char* path);


void register_internal_funcs( void );

void engine_builtinFunction(http_request *s,FUNCTION_PARAS* func);
int check_platformFunction_exists(FUNCTION_PARAS* func);
void engine_platformFunction(http_request *s,FUNCTION_PARAS* func);
void engine_includeFile(http_request *s,const char* prefix,FUNCTION_PARAS* func);


void engine_getVariable( http_request *s,FUNCTION_PARAS* func);
void engine_setVariable( http_request *s,FUNCTION_PARAS* func);
void engine_loop_array ( http_request *s,const char* prefix,const char *pagename,const char *pagedata,const int datalenght,FUNCTION_PARAS* func,int* i );
void engine_TemplateIF( http_request *s,const char* prefix,const char *pagename,const char *pagedata,const int datalenght,FUNCTION_PARAS* func,int* i);

int  handleWebsocketConnection ( WEBSOCKET_SIGNALS signal,const char* guid,const char* url,const char binary,const char* msg, const unsigned long long len);

CONDITION_RETURN engine_callCondition ( http_request *s,FUNCTION_PARAS* func );

void printRegisteredFunctions ( http_request* s );
void printRegisteredConditions(http_request* s) ;
void printRegisteredPlugins ( http_request* s ) ;

void RegisterPluginErrorHandler(plugin_error_handler f);

void DummyFuncConst( UNUSED_PARA const void* a);
void DummyFunc( UNUSED_PARA void* a);
int StrKeyComp( UNUSED_PARA const void* a, UNUSED_PARA const void* b);

#ifdef __cplusplus
}
#endif

#endif

