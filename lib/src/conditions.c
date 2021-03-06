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

static CONDITION_RETURN is_user_registered(http_request* s) {
	int i = checkUserRegistered(s);
	if ((i == NORMAL_CHECK_OK) || (i == SSL_CHECK_OK)){
		return CONDITION_TRUE;
	}
	return CONDITION_FALSE;
}

static CONDITION_RETURN is_user_not_registered(http_request* s) {
	int i = checkUserRegistered(s);
	if ((i == NORMAL_CHECK_OK) || (i == SSL_CHECK_OK)){
		return CONDITION_FALSE;
	}
	return CONDITION_TRUE;
}

static CONDITION_RETURN is_user_registered_ssl(http_request* s) {
#ifdef WEBSERVER_USE_SSL
	int i = checkUserRegistered(s);
	if (i == SSL_CHECK_OK ){
		return CONDITION_TRUE;
	}
#endif
	return CONDITION_FALSE;
}

static CONDITION_RETURN is_user_not_registered_ssl(http_request* s) {
#ifdef WEBSERVER_USE_SSL
	int i = checkUserRegistered(s);
	if (i == SSL_CHECK_OK ){
		return CONDITION_FALSE;
	}
#endif
	return CONDITION_TRUE;
}

static CONDITION_RETURN equal_condition(http_request* s, FUNCTION_PARAS* func) {
	ws_variable *op1, *var1;
	ws_variable *op2, *var2;
	CONDITION_RETURN ret = CONDITION_FALSE;

	op1 = parseVariable(s, func->parameter[1].text);
	op2 = parseVariable(s, func->parameter[2].text);

	if ((op1 == 0) || (op2 == 0)) {

		//if (op1 != 0)
			//if (0 == strcmp(op1->name, "op")) /* erzeugte Variable */
				freeWSVariable(op1);

		//if (op2 != 0)
			//if (0 == strcmp(op2->name, "op")) /* erzeugte Variable */
				freeWSVariable(op2);
#ifdef ENABLE_DEVEL_WARNINGS
		#warning prüfen ob die checks eine bedeutung hatten
#endif

		return CONDITION_FALSE;
	}

	if (op1->type == VAR_TYPE_REF){
		var1 = op1->val.value_ref;
	}else{
		var1 = op1;
	}

	if (op2->type == VAR_TYPE_REF){
		var2 = op2->val.value_ref;
	}else{
		var2 = op2;
	}

	if (var1->type == var2->type) {
		if (var1->type == VAR_TYPE_INT){
			if (var1->val.value_int == var2->val.value_int){
				ret = CONDITION_TRUE;
			}
		}

		if (var1->type == VAR_TYPE_STRING){
			if (0 == strcmp(var1->val.value_string, var2->val.value_string)){
				ret = CONDITION_TRUE;
			}
		}
	}

	if ((var1->type == VAR_TYPE_INT) && (var2->type == VAR_TYPE_STRING)) {
		if (var1->val.value_int == getWSVariableInt(var2)){
			ret = CONDITION_TRUE;
		}

	}

	if ((var2->type == VAR_TYPE_INT) && (var1->type == VAR_TYPE_STRING)) {
		if (var2->val.value_int == getWSVariableInt(var1)){
			ret = CONDITION_TRUE;
		}
	}

	freeWSVariable(op1);
	freeWSVariable(op2);

	return ret;

}

#define CheckCondition(a,b) if (0==strncmp(func->parameter[0].text,a,strlen(a)))	if(func->parameter[0].text[strlen(a)]==0) return b(s);

CONDITION_RETURN builtinConditions(http_request* s, FUNCTION_PARAS* func) {
	CONDITION_RETURN tmp;
	CheckCondition( "is_user_registered", is_user_registered)
	CheckCondition( "is_user_not_registered", is_user_not_registered)

	CheckCondition( "is_user_registered_ssl", is_user_registered_ssl)
	CheckCondition( "is_user_not_registered_ssl", is_user_not_registered_ssl)

	if (0 == strcmp( func->parameter[0].text, "is_false")) {
		return CONDITION_FALSE;
	}

	if (0 == strcmp( func->parameter[0].text, "is_true")) {
		return CONDITION_TRUE;
	}

	if (0 == strncmp( func->parameter[0].text, "eq", 2)) {
		return equal_condition(s, func);
	}
	if (0 == strncmp( func->parameter[0].text, "!eq", 2)) {
		tmp = equal_condition(s, func);
		if (tmp == CONDITION_TRUE){
			return CONDITION_FALSE;
		}
		if (tmp == CONDITION_FALSE){
			return CONDITION_TRUE;
		}
	}

	if (0 == strcmp(func->parameter[0].text, "is_ssl_active")) {
#ifdef WEBSERVER_USE_SSL
		if (s->socket->use_ssl == 1){
			return CONDITION_TRUE;
		}else{
#endif
			return CONDITION_FALSE;
#ifdef WEBSERVER_USE_SSL
		}
#endif
	}

	if (0 == strcmp(func->parameter[0].text, "is_websockets_available")) {
#ifdef WEBSERVER_USE_WEBSOCKETS
		return CONDITION_TRUE;
#else
		return CONDITION_FALSE;
#endif
	}

	if (0 == strcmp(func->parameter[0].text, "is_ssl_available")) {
#ifdef WEBSERVER_USE_SSL
		return CONDITION_TRUE;
#else
		return CONDITION_FALSE;
#endif
	}

	if (0 == strcmp(func->parameter[0].text, "is_64bit")) {
#ifdef __GNUC__
#ifdef __LP64__
		return CONDITION_TRUE;
#else
		return CONDITION_FALSE;
#endif
#endif

#ifdef __INTEL_COMPILER
#ifdef _WIN64
		return CONDITION_TRUE;
#else
		return CONDITION_FALSE;
#endif
#endif

#ifdef _MSC_VER
#ifdef _WIN64
		return CONDITION_TRUE;
#else
		return CONDITION_FALSE;
#endif
#endif
	}

	LOG( TEMPLATE_LOG, ERROR_LEVEL, s->socket->socket, "Unknown Condition %s\n", func->parameter[0].text);
	return CONDITION_ERROR;
}
