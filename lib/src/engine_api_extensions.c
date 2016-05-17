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
#endif


#include "webserver.h"
#include "red_black_tree.h"
#include "webserver_api_functions.h"


#ifdef DMALLOC
#include <dmalloc/dmalloc.h>
#endif



rb_red_blk_tree *user_func_tree;
rb_red_blk_tree *user_condition_tree;
rb_red_blk_tree *websocket_handler_tree;

list_t plugin_liste;
plugin_error_handler error_handler = 0;

plugin_s *current_plugin = 0;

void DummyFunc( UNUSED_PARA void* a) {
}
void DummyFuncConst( UNUSED_PARA const void* a) {
}

int StrKeyComp(const void* a, const void* b) {
	int ret;
	char *p_a = (char*) a;
	char *p_b = (char*) b;
	ret = strcmp(p_a, p_b);
	if (ret < 0)
		return -1;
	if (ret > 0)
		return 1;
	return 0;
}

void init_extension_api(void) {

#ifdef WEBSERVER_USE_WEBSOCKETS
	initWebsocketApi();
#endif

	user_func_tree = RBTreeCreate(StrKeyComp, DummyFunc, DummyFunc, DummyFuncConst, DummyFunc);
	user_condition_tree = RBTreeCreate(StrKeyComp, DummyFunc, DummyFunc, DummyFuncConst, DummyFunc);

	websocket_handler_tree = RBTreeCreate(StrKeyComp, DummyFunc, DummyFunc, DummyFuncConst, DummyFunc);

	ws_list_init(&plugin_liste);

}

void free_extension_api(void) {
	RBTreeDestroy(user_func_tree);
	RBTreeDestroy(user_condition_tree);
	RBTreeDestroy(websocket_handler_tree);
	ws_list_destroy(&plugin_liste);
}

void register_function(const char* name, user_function f, const char* file, int line) {
	int len = 0;
	user_func_s *tmp = (user_func_s*) WebserverMalloc( sizeof(user_func_s) );
	len = strlen(name) + 1;
	tmp->name = (char*) WebserverMalloc( len );
	strncpy(tmp->name, name, len);

	tmp->file = file;
	tmp->line = line;
	tmp->plugin = current_plugin;

	tmp->type = 0;
	tmp->uf = f;

	RBTreeInsert(user_func_tree, tmp->name, tmp);
}

#ifdef WEBSERVER_USE_PYTHON
void register_py_function( const char* name, PyObject * py_func, const char* file, int line) {
	int len = 0;

	user_func_s *tmp ;

	rb_red_blk_node* node = RBExactQuery(user_func_tree, (char*)name);
	if (node != 0) {
		tmp = node->info;
		if ( tmp->type == 1 ) {
			printf("reregister PyFunc: %s name \n",name);
			tmp->file = file;
			tmp->line = line;

			Py_XDECREF(tmp->py_func);

			tmp->py_func = py_func;
			return;
		}
	}

	tmp = (user_func_s*) WebserverMalloc( sizeof(user_func_s) );

	len = strlen(name) + 1;
	tmp->name = (char*) WebserverMalloc( len );
	strncpy(tmp->name, name, len);

	tmp->file = file;
	tmp->line = line;
	tmp->plugin = current_plugin;

	tmp->type = 1;
	tmp->py_func = py_func;

	RBTreeInsert(user_func_tree, tmp->name, tmp);
}
#endif

int check_platformFunction_exists(FUNCTION_PARAS* func) {
	rb_red_blk_node* node;

	if (func->parameter[0].text == 0){
		LOG(TEMPLATE_LOG, INFO_LEVEL, 0, "Platform Function Parameter 0 == 0","");
		return 1;
	}

	node = RBExactQuery(user_func_tree, func->parameter[0].text);
	if (node == 0) {
		func->platform_function = 0;
		LOG(TEMPLATE_LOG, INFO_LEVEL, 0, "Platform Function %s not found", func->parameter[0].text);
		return 1;
	}
	func->platform_function = node;
	return 0;
}

void engine_platformFunction(http_request *s, FUNCTION_PARAS* func) {
	user_func_s *tmp;
	rb_red_blk_node* node;
	char dummy[] = "compiled in";
       	char *name;

	if (func->parameter[0].text == 0)
		return;
	node = func->platform_function;

	tmp = (user_func_s*) node->info;
#ifdef _WEBSERVER_ENGINE_PLUGINS_DEBUG_
	LOG( TEMPLATE_LOG, NOTICE_LEVEL, s->socket->socket, "Calling Plugin Function %s enter", func->para[0]);
#endif

	if ((error_handler != 0) && (tmp->plugin != 0))
		name = tmp->plugin->name;
	else
		name = dummy;

	if (error_handler != 0) {
		error_handler(PLUGIN_FUNCTION_CALLING, name, func->parameter[0].text, "crashed");
	}

	if ( tmp->type == 0 )
		tmp->uf(s, func);

#ifdef WEBSERVER_USE_PYTHON
	if ( tmp->type == 1 )
		py_call_engine_function( s, tmp, func );
#endif

	if (error_handler != 0) {
		error_handler(PLUGIN_FUNCTION_CALLED, name, func->parameter[0].text, "crashed");
	}

#ifdef _WEBSERVER_ENGINE_PLUGINS_DEBUG_
	LOG( TEMPLATE_LOG, NOTICE_LEVEL, s->socket->socket, "Calling Plugin Function %s exit", func->para[0]);
#endif

}


void printRegisteredFunctions(http_request* s) {
	stk_stack* stack;
	rb_red_blk_node* node;
	user_func_s *uf;
	int status = 0;
	char buffer[100];

	ws_variable* var1 = getParameter(s, "reset_functions");

	if (var1 != 0) {
		error_handler(PLUGIN_FUNCTION_RESET_CRASHED, "", "", "");
	}

	stack = RBEnumerate(user_func_tree, (void*)"0", (void*)"z");
	while (0 != StackNotEmpty(stack)) {
		node = (rb_red_blk_node*) StackPop(stack);
		uf = (user_func_s*) node->info;

		if (error_handler != 0) {
			if (uf->plugin == 0){
				status = error_handler(PLUGIN_FUNCTION_CHECK_ERROR, "internal", uf->name, "crashed");
			}
		}

		if (status == 0){
			sprintf(buffer, "<FONT color=green>ok</font>");
		}else{
			sprintf(buffer, "<FONT color=red>crashed</font>");
		}




		if (uf->plugin == 0){
			printHTMLChunk(s->socket, "<tr><td>internal<td>%s<td>%s<td>%s<td>%d<td>%s", uf->name, "C", uf->file, uf->line, buffer);
		}

	}
	free(stack);

	stack = RBEnumerate(user_func_tree, (void*)"0", (void*)"z");
	while (0 != StackNotEmpty(stack)) {
		node = (rb_red_blk_node*) StackPop(stack);
		uf = (user_func_s*) node->info;

		if (error_handler != 0) {
			if (uf->plugin != 0){
				status = error_handler(PLUGIN_FUNCTION_CHECK_ERROR, uf->plugin->name, uf->name, "crashed");
			}
		}

		if (status == 0){
			sprintf(buffer, "<FONT color=green>ok</font>");
		}else{
			sprintf(buffer, "<FONT color=red>crashed</font>");
		}

		char type[10];
		if( uf->type == 0 ) sprintf(type,"C");
		if( uf->type == 1 ) sprintf(type,"Python");

		if (uf->plugin != 0){
			printHTMLChunk(s->socket, "<tr><td>%s<td>%s<td>%s<td>%s<td>%d<td>%s", uf->plugin->name,  uf->name, type, uf->file, uf->line, buffer);
		}
	}
	free(stack);
}

void printRegisteredConditions(http_request* s) {
	stk_stack* stack;
	rb_red_blk_node* node;
	user_condition_s *uc;
	int status = 0;
	char buffer[100];

	stack = RBEnumerate(user_condition_tree, (void*)"0", (void*)"z");
	while (0 != StackNotEmpty(stack)) {
		node = (rb_red_blk_node*) StackPop(stack);
		uc = (user_condition_s*) node->info;

		if (error_handler != 0) {
			if (uc->plugin == 0)
				status = error_handler(PLUGIN_FUNCTION_CHECK_ERROR, "internal", uc->name, "crashed");
			else
				status = error_handler(PLUGIN_FUNCTION_CHECK_ERROR, uc->plugin->name, uc->name, "crashed");
		}

		if (status == 0)
			sprintf(buffer, "<FONT color=green>ok</font>");
		else
			sprintf(buffer, "<FONT color=red>crashed</font>");

		if (uc->plugin == 0)
			printHTMLChunk(s->socket, "<tr><td>internal<td>%s<td>%s<td>%d<td>%s", uc->name, uc->file, uc->line, buffer);
		else
			printHTMLChunk(s->socket, "<tr><td>%s<td>%s<td>%s<td>%d<td>%s", uc->plugin->name, uc->name, uc->file, uc->line, buffer);
	}
	free(stack);
}

CONDITION_RETURN engine_callCondition(http_request *s, FUNCTION_PARAS* func) {
	user_condition_s *tmp;
	CONDITION_RETURN cond_ret = CONDITION_ERROR;

	rb_red_blk_node* node = RBExactQuery(user_condition_tree, func->parameter[0].text);
	if (node != 0) {
		tmp = (user_condition_s*) node->info;
		cond_ret = tmp->uc(s, func);
	}

	if (cond_ret == CONDITION_ERROR) {
		cond_ret = builtinConditions(s, func);
	}

	if (cond_ret == CONDITION_ERROR) {
		LOG(TEMPLATE_LOG, NOTICE_LEVEL, s->socket->socket, "Condition %s not found", func->parameter[0].text);
	}
	return cond_ret;
}

void register_condition(const char* name, user_condition f, const char* file, int line) {
	int len = 0;
	user_condition_s *tmp = (user_condition_s*) WebserverMalloc( sizeof(user_condition_s) );
	len = strlen(name) + 1;
	tmp->name = (char*) WebserverMalloc( len );
	strncpy(tmp->name, name, len);
	tmp->uc = f;
	tmp->file = file;
	tmp->line = line;
	tmp->plugin = current_plugin;
	RBTreeInsert(user_condition_tree, tmp->name, tmp);
}


/*
	http://ctpp.havoc.ru/doc/en/


 Kein Locking fuer Websocket Handler weil Plugins im init_hook handler registrieren muessen
*/

void register_function_websocket_handler(const char* url, websocket_handler f, const char* file, int line) {
	websocket_handler_s *tmp;
	rb_red_blk_node* node = RBExactQuery(websocket_handler_tree, (char*) url);
	websocket_handler_list_s *list;

	if (node == 0) {
		list = (websocket_handler_list_s*) WebserverMalloc( sizeof(websocket_handler_list_s) );
		list->handler_list = (list_t*) WebserverMalloc( sizeof(list_t) );
		ws_list_init(list->handler_list);
		list->url = (char*) WebserverMalloc( strlen(url) + 1 );
		strcpy(list->url, url);
		RBTreeInsert(websocket_handler_tree, list->url, list);
	} else {
		list = (websocket_handler_list_s*) node->info;
	}

	tmp = (websocket_handler_s*) WebserverMalloc( sizeof(websocket_handler_s) );
	tmp->wsh = f;
	tmp->file = file;
	tmp->line = line;
	ws_list_append(list->handler_list, tmp);

}

int handleWebsocketConnection(WEBSOCKET_SIGNALS signal, const char* guid, const char* url, const char binary , const char* msg, const unsigned long long len) {
	websocket_handler_s *handler;
	websocket_handler_list_s *list;
	rb_red_blk_node* node = RBExactQuery(websocket_handler_tree, (char*) url);
	if (node != 0) {
		list = (websocket_handler_list_s*) node->info;
		ws_list_iterator_start(list->handler_list);
		while (ws_list_iterator_hasnext(list->handler_list)) {
			handler = (websocket_handler_s*) ws_list_iterator_next(list->handler_list);
			handler->wsh(signal, guid,binary, msg, len);
		}
		ws_list_iterator_stop(list->handler_list);
	}
	return 0;
}

char* (*init_webserver_plugin)(void);
int (*get_webserver_api_version)(void);

#define COPY_ERROR(a) \
		plugin->error = (char*) WebserverMalloc( strlen(a) +1 ); \
		strcpy(plugin->error, a);

int loadPlugin(const char* name, const char* path) {
	int version;
	char *error_text;
	char ldd_buffer[1000];
	void *dl;

#ifdef _WIN32
#pragma message ( "loadPlugin not implemented on WIN32" )
#elif defined( __GNUC__ )

	plugin_s *plugin, *plugin_tmp;

	/* Pruefen on Plugin schon geladen wurde */
	ws_list_iterator_start(&plugin_liste);
	while ((plugin_tmp = (plugin_s*) ws_list_iterator_next(&plugin_liste))) {
		if (0 == strcmp(plugin_tmp->path, path)) {
			printf("Plugin %s already exists\n", path);
			ws_list_iterator_stop(&plugin_liste);
			return -1;
		}
	}
	ws_list_iterator_stop(&plugin_liste);

	/* Plugin Struktur erzeugen */
	plugin = (plugin_s*) WebserverMalloc( sizeof(plugin_s) );
	plugin->name = (char*) WebserverMalloc( strlen(name) + 1 );
	strcpy(plugin->name, name);
	plugin->path = (char*) WebserverMalloc( strlen(path) + 1 );
	strcpy(plugin->path, path);

	ws_list_append(&plugin_liste, plugin);

	/* Prueen ob die Plugin Init Funktion abgestuertzt ist */
	if (error_handler != 0) {
		if (0 != error_handler(PLUGIN_CHECK_ERROR, name, "", "")) {
			COPY_ERROR("crashed")
			return -6;
		}
	}

	/* Plugin Shared Object laden */
#ifdef RTLD_DEEPBIND
	/* dlopen(path, RTLD_NOW | RTLD_GLOBAL | RTLD_DEEPBIND ); */
	dl = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
#else
	dl = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
#endif
	error_text = dlerror();
	if (dl == 0) {
		if (error_handler != 0) {
			error_handler(PLUGIN_LOADING, name, "", "dlopen Error");
		}
		COPY_ERROR("dlopen Error")
		printf("Error loading %s %s\n", path, error_text);
		sprintf(ldd_buffer,"ldd %s",path);
		system( ldd_buffer );
		return -2;
	}

	/* Funktion zum abfragen der API Version suchen */
	get_webserver_api_version = (int (*)(void)) dlsym(dl, "get_webserver_api_version"); /* hier muss das plugin seine template funktionen registrieren */
	error_text = dlerror();

	if (get_webserver_api_version == 0) {
		if (error_handler != 0) {
			error_handler(PLUGIN_LOADING, name, "", "No get_webserver_api_version Function");
		}
		COPY_ERROR("No get_webserver_api_version Function")
		printf("Error loading %s. No Init Function char* get_webserver_api_version(void) %s\n", path, error_text);
		dlclose( dl );
		return -3;
	}

	/* API Version abfragen */
	version = (*get_webserver_api_version)();
	if (version > WEBSERVER_API) {
		if (error_handler != 0) {
			error_handler(PLUGIN_LOADING, name, "", "API Version inkompatibel");
		}
		COPY_ERROR("API Version inkompatibel")
		LOG( TEMPLATE_LOG, ERROR_LEVEL, 0, "Plugin API Version inkompatibel %s %s", path, error_text);
		dlclose( dl );
		return -4;
	}

	/* Plugin Init Funktion suchen */
	init_webserver_plugin = (char *(*)(void)) dlsym(dl, "init_webserver_plugin"); /* hier muss das plugin seine template funktionen registrieren */
	error_text = dlerror();

	if (init_webserver_plugin == 0) {
		if (error_handler != 0) {
			error_handler(PLUGIN_LOADING, name, "", "No init_webserver_plugin Function");
		}
		COPY_ERROR("No init_webserver_plugin Function")
		printf("Error loading %s. No Init Function char* init_webserver_plugin(void) %s \n", path, error_text);
		return -5;
	}


	if (error_handler != 0) {
		error_handler(PLUGIN_LOADING, name, "", "crashed on init");
	}

	current_plugin = plugin;

	plugin->type = 0;

	(*init_webserver_plugin)();

	current_plugin = 0;

	if (error_handler != 0) {
		error_handler(PLUGIN_LOADED, name, "", "crashed on init");
	}

	COPY_ERROR("loaded")

	return 0;
#else
#error "loadPlugin not implemented in system"
#endif
}

void RegisterPluginErrorHandler(plugin_error_handler f) {
	error_handler = f;
}

void printRegisteredPlugins(http_request* s) {
	plugin_s *p = 0;

	ws_variable *var1;

	var1 = getParameter(s, "reset_plugins");
	if (var1 != 0) {
		error_handler(PLUGIN_RESET_CRASHED, "", "", "");
	}

	ws_list_iterator_start(&plugin_liste);
	while ((p = (plugin_s*) ws_list_iterator_next(&plugin_liste))) {
		char type[10];
		if( p->type == 0 ) sprintf(type,"C");
		if( p->type == 1 ) sprintf(type,"Python");

		if (0 == strcmp(p->error, "crashed"))
			printHTMLChunk(s->socket, "<tr><td>%s<td>%s<td>%s<td><font color=red>%s</font>", p->name, p->path, type, p->error);
		else
			printHTMLChunk(s->socket, "<tr><td>%s<td>%s<td>%s<td><font color=green>%s</font>", p->name, p->path, type, p->error);
	}
	ws_list_iterator_stop(&plugin_liste);
}

