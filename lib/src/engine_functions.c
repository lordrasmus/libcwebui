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

#ifdef __GNUC__
	#include <dlfcn.h>
	#include "webserver.h"
#endif

#ifdef DMALLOC
#include <dmalloc/dmalloc.h>
#endif






void engine_includeFile(http_request *s, const char* prefix, FUNCTION_PARAS* func) {
	WebserverFileInfo *file;

	if ( func->parameter[0].text == 0 )
		return;

	file = getFile(func->parameter[0].text);
	if (file == 0) {
		#ifdef _WEBSERVER_TEMPLATE_DEBUG_
		WebServerPrintf("File : %s not found\n",func->para1);
		#endif
		printHTMLChunk(s->socket, "<font color=\"#FF0000\" >include %s not found</font>", func->parameter[0].text);
		return;
	}

	if ( 0 == prepare_file_content( file ) ){
		#ifdef _WEBSERVER_TEMPLATE_DEBUG_
		WebServerPrintf("File : %s error loading content \n",func->para1);
		#endif

		printHTMLChunk(s->socket, "<font color=\"#FF0000\" >include %s read error</font>", func->parameter[0].text);
		return;
	}

	if ( file->TemplateFile == 0){
		printf("Warning Engine Template Header <%s> not found: %s \n",template_v1_header,file->FilePath);
	}

	processHTML(s, prefix, file->Url, (char*) file->Data, file->DataLenght);

	release_file_content( file );

}



void engine_printWSVar(http_request *s, ws_variable* var) {
	ws_variable *tmp;
	switch (var->type) {
	case VAR_TYPE_STRING:
		sendHTMLChunk(s->socket, var->val.value_string, var->extra.str_len);
		break;
	case VAR_TYPE_INT:
		printHTMLChunk(s->socket, "%d", var->val.value_int);
		break;
	case VAR_TYPE_ARRAY:
		printHTMLChunk(s->socket, "%s Array", var->name);
		break;
	case VAR_TYPE_REF:
		tmp = var->val.value_ref;
		engine_printWSVar(s, tmp);
		break;
	default:
		printHTMLChunk(s->socket, "<font color=\"#FF0000\" >engine_printWSVar : %s Unsuportet Type %d</font> ", var->name, var->type);
		break;
	}
}

void engine_getVariable(http_request *s, FUNCTION_PARAS* func) {
	ws_variable* var = parseVariable(s, func->parameter[0].text);
	sendHTMLChunkVariable(s->socket, var);
	freeWSVariable(var);
}

void engine_setVariable(http_request *s, FUNCTION_PARAS* func) {
	ws_variable* var = parseVariable(s, func->parameter[0].text);
	ws_variable* var2;
	ws_variable* set_value_var;

	if ( var == 0 ) return;

	set_value_var = parseVariable(s, func->parameter[1].text);

	if ( set_value_var == 0){
		freeWSVariable( var );
		return;
	}

	if (var->type != VAR_TYPE_REF) {
		var2 = getRenderVariable(s, var->name);
		freeWSVariable(var);
		var = var2;
		switch (set_value_var->type) {
		case VAR_TYPE_STRING:
			setWSVariableString(var, set_value_var->val.value_string);
			break;
		case VAR_TYPE_INT:
			setWSVariableInt(var, set_value_var->val.value_int);
			break;
		default:
			printHTMLChunk(s->socket,"<font color=\"#FF0000\" >engine_setVariable Typ %d noch behandeln</font>",set_value_var->type);
			break;
		}
		freeWSVariable(set_value_var);

		return;
	} else {
		switch (set_value_var->type) {
		case VAR_TYPE_STRING:
			setWSVariableString(var->val.value_ref, set_value_var->val.value_string);
			break;
		case VAR_TYPE_INT:
			setWSVariableInt(var->val.value_ref, set_value_var->val.value_int);
			break;
		default:
			printf("engine_setVariable Typ %d noch behandeln\n",set_value_var->type);
			break;
		}
		freeWSVariable(set_value_var);
		freeWSVariable(var);
		return;
	}
}

void engine_builtinFunction(http_request *s, FUNCTION_PARAS* func) {
	builtinFunction(s, func);
}

