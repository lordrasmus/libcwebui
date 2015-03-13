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

#include "webserver.h"

#pragma GCC visibility push(default)


void WebserverGenerateGUID(char* buf, int length){
	generateGUID(buf,length);
}

void register_engine_function(const char* name, user_api_function f,
		const char* file, int line) {
	//void register_function(const char* name,user_function f,const char* file,int line)
	register_function(name, (user_function) f, file, line);
}

void register_engine_condition(const char* name, user_api_condition f,
		const char* file, int line) {
	//void register_condition(const char* name,user_condition f,const char* file,int line)
	register_condition(name, (user_condition) f, file, line);
}

void register_websocket_handler(const char* url, websocket_api_handler f,
		const char* file, int line) {
	//void register_function_websocket_handler(const char* url,websocket_handler f,const char* file,int line)
	register_function_websocket_handler(url, (websocket_handler) f, file, line);
}

/*****************************************************************************
 *
 *						Ausgabe Funktionen
 *
 *****************************************************************************/

void sendHTMLVariable(dummy_handler* s, dummy_var* var) {
	sendHTMLChunkVariable(((http_request*) s)->socket, (ws_variable*) var);
}

// printHTMLChunk ( socket_info* sock,const char *fmt,... )
void printHTML(dummy_handler* s, const char *fmt, ...) {
	va_list arg;
	va_start ( arg, fmt );
	vprintHTMLChunk(((http_request*)s)->socket,fmt,arg);
	va_end ( arg );
}

void sendHTML(dummy_handler* s, const char* text, const unsigned int length){
	//void sendHTMLChunk(socket_info* sock, const char* text, const unsigned int length)
	sendHTMLChunk( ((http_request*) s)->socket, text, length);
}


void addFireLogger(dummy_handler* s, const char* filename, int fileline, const char* fmt, ... ) {
	//void addFirePHPLog ( http_request* s,char* filename,int fileline,char* text,... )
	va_list arg;
	va_start ( arg, fmt );
	vaddFirePHPLog((http_request*) s,filename,fileline,fmt,arg);
	va_end ( arg );

}

/*****************************************************************************
 *
 *						Variablen
 *
 *****************************************************************************/

char* WebsocketGetStoreGUID(char* guid){
	return getWebsocketStoreGUID(guid);
}


char setSessionVarGUID(char* store_guid, int store, const char* name, const char* value){
	return setSessionValueByGUID(store_guid, (STORE_TYPES)store, name, value);
}

dummy_var* getSessionVarGUID(char* store_guid, int store, const char* name, UNUSED_PARA WS_VAR_FLAGS flags){
	return (dummy_var*) getSessionValueByGUID(store_guid, (STORE_TYPES)store, name);
}

char* getSessionGUID(dummy_handler* s){
	return ((http_request*) s)->guid;

}


char setSessionVar(dummy_handler* s, int store, const char* name, const char* value) {
	// char setSessionValue(http_request* s,STORE_TYPES store,const char* name,const char* value)
	return setSessionValue((http_request*) s, (STORE_TYPES) store, name, value);
}

dummy_var* getSessionVar(dummy_handler* s, int store, const char* name,UNUSED_PARA WS_VAR_FLAGS flags) {
	// ws_variable* getSessionValue(http_request* s,STORE_TYPES store,const char* name)
	ws_variable* ret = getSessionValue((http_request*) s, (STORE_TYPES) store,	name);

	if ( (  flags & DO_NOT_CREATE ) == 0 ){
		if (ret == 0){
			ret = addSessionValue((http_request*)s,(STORE_TYPES) store, name);
		}
	}
	return (dummy_var*) ret;
}

dummy_var* getRenderVar(dummy_handler* s, const char* name,WS_VAR_FLAGS flags) {
	ws_variable* ret = getVariable(((http_request*)s)->render_var_store,name);
	if ( (  flags & DO_NOT_CREATE ) == 0 ){
		if (ret == 0){
			ret = newVariable(((http_request*)s)->render_var_store,name);
		}
	}
	return (dummy_var*) ret;
}

void setRenderVar(dummy_handler* s, char* name, char* text) {
	setRenderVariable((http_request*) s, name, text);
}

void setRenderVar_v(dummy_handler* s, char* name, char* format,...) {
	va_list arg;

	va_start(arg, format);
	char buffer[vsnprintf(0, 0, format, arg) + 1];
	va_end(arg);

	va_start(arg, format);
	vsprintf(buffer, format, arg);
	va_end(arg);

	setRenderVariable((http_request*) s, name, buffer);
}


dummy_var* getGlobalVar(const char* name,WS_VAR_FLAGS flags) {
	if ( (  flags & DO_NOT_CREATE ) == DO_NOT_CREATE ){
		return (dummy_var*) getExistentGlobalVariable(name);
	}else{
		return (dummy_var*) getGlobalVariable(name);
	}
}

dummy_var* setGlobalVar(const char* name, const char* text) {
	return (dummy_var*) setGlobalVariable(name, text);
}

void setVariableToArray(dummy_var* var) {
	setWSVariableArray((ws_variable*) var);
}

dummy_var* addToVarArray(dummy_var* var, const char* name) {
	return (dummy_var*) addWSVariableArray((ws_variable*) var, name);
}

dummy_var* addToVarArrayCustomData(dummy_var* var, const char* name,free_handler handle, void* data ){
	ws_variable* ret = addWSVariableArray((ws_variable*) var, name);
	setWSVariableCustomData(ret, handle, data);
	return (dummy_var*)ret;
}

void* getVariableAsCustomData(dummy_var* var){
	ws_variable* var2 = (ws_variable*) var;
	return var2->val.value_p;
}

void addToVarArrayInt(dummy_var* var, const char* name,int value){
	dummy_var* var2 = addToVarArray(var, name);
	setVariableAsInt(var2, value);
}

void addToVarArrayStr(dummy_var* var, const char* name, char* buffer ){
	dummy_var* var2 = addToVarArray(var, name);
	setVariableAsString(var2, buffer );
}

int getVariableAsString(dummy_var* var, char* buffer, unsigned int buffer_length) {
	return getWSVariableString((ws_variable*) var, buffer, buffer_length);
}

char* getVariableAsStringP(dummy_var* var){
	ws_variable* var2 = (ws_variable*) var;
	if ( var2->type == VAR_TYPE_STRING )
		return var2->val.value_string;
	return 0;
}

void setVariableAsString(dummy_var* var, const char* text) {
	setWSVariableString((ws_variable*) var, text);
}

int getVariableAsInt(dummy_var* var) {
	return getWSVariableInt((ws_variable*) var);
}

void setVariableAsInt(dummy_var* var, int value){
	setWSVariableInt((ws_variable*) var, value);
}

uint64_t getVariableAsULong(dummy_var* var){
	return getWSVariableULong((ws_variable*) var);
}

void setVariableAsULong(dummy_var* var, uint64_t value){
	setWSVariableULong((ws_variable*) var, value);
}

void setVariableToRef(dummy_var* var,dummy_var* ref){
	setWSVariableRef((ws_variable*)var,(ws_variable*)ref);
}

int lockGlobalVars(void){
	return lockGlobals();
}
int unlockGlobalVars(void){
	return unlockGlobals();
}

dummy_var*	getArrayVariable(dummy_var* var, const char* name){
	return (dummy_var*)getWSVariableArray( (ws_variable*) var ,name);
}

dummy_var*	getArrayFirst(dummy_var* var){
	return (dummy_var*)	getWSVariableArrayFirst((ws_variable*) var);

}
dummy_var*	getArrayNext(dummy_var* var){
	return (dummy_var*) getWSVariableArrayNext((ws_variable*) var);

}
void 		stopArrayIterate(dummy_var* var){
	stopWSVariableArrayIterate((ws_variable*) var);
}



/*****************************************************************************
 *
 *						Authentifizierung über Cookies
 *
 *****************************************************************************/

char setUserRegisterStatus(dummy_handler* s, char status) {
	//char setUserRegistered(http_request* s,char status)
	return setUserRegistered((http_request*) s, status);
}

int checkUserRegisterStatus(dummy_handler* s) {
	//int checkUserRegistered(http_request* s) {
	return checkUserRegistered((http_request*) s);
}

/***********************************************************************************************
*                                                                                              *
*          POST File Upload                                                                    *
*                                                                                              *
************************************************************************************************/

int  getFileCount(dummy_handler* s){
	return ws_list_size( &((http_request*) s)->upload_files);
}

char*     getFileName(dummy_handler* s,int index){
	upload_file_info* tmp = ws_list_get_at( &((http_request*) s)->upload_files, index );
	return tmp->name;
}

char*     getFileData(dummy_handler* s,int index){
	upload_file_info* tmp = ws_list_get_at( &((http_request*) s)->upload_files, index );
	if ( ! tmp ) return 0;
	return tmp->data;
}

int     getFileSize(dummy_handler* s,int index){
	upload_file_info* tmp = ws_list_get_at( &((http_request*) s)->upload_files, index );
	if ( ! tmp ) return 0;
	return tmp->length;
}

char*     getPostData(dummy_handler* s){
	return ((http_request*) s)->header->post_buffer;
}

/*****************************************************************************
 *
 *						Request Status Informationen
 *
 *****************************************************************************/

char *getRequestURL(dummy_handler* s) {
	http_request* s1 = (http_request*) s;
	return s1->header->url;
}

char isRequestSecure(dummy_handler *s) {
	return ((http_request*) s)->socket->use_ssl;
}

dummy_var* getURLParameter(dummy_handler* s,const char* name) {
	return (dummy_var*) getParameter((http_request*) s, name);
}

char* getEngineParameter(dummy_handler* s, int index) {
	//int i;
	FUNCTION_PARAS* f = &((http_request*)s)->engine_current->func;
	if ( index >= MAX_FUNC_PARAS ) return 0;

	return f->parameter[index].text;
}

void  dumpEngineParameter(dummy_handler* s){
	for( int i = 0; i < MAX_FUNC_PARAS ; i++ ){

		if (getEngineParameter(s, i) != 0){
			printHTML(s,"Para%d : %s<br>", i, getEngineParameter(s, i));
		}
	}
}

int WebsocketSendTextFrame(const char* guid, const char* in, const int length) {
#ifdef WEBSERVER_USE_WEBSOCKETS
	return sendWebsocketTextFrame(guid, in, length);
#else
	return 0;
#endif
}

int WebsocketSendBinaryFrame(const char* guid, const char* in, const int length) {
#ifdef WEBSERVER_USE_WEBSOCKETS
	return sendWebsocketBinaryFrame(guid, (const unsigned char*)in, length);
#else
	return 0;
#endif
}

int WebsocketSendCloseFrame(const char* guid){
	sendCloseFrame2(guid);
	return 0;
}

int  WebserverLoadPlugin(const char* name,const char* path){
	return loadPlugin(name,path);
}

void RegisterEngineFunction(const char* name,user_api_function f,const char* file,int line){
	register_engine_function(name,f,file,line);
}
void RegisterEngineCondition(const char* name,user_api_condition f,const char* file,int line){
	register_engine_condition(name,f,file,line);
}
void RegisterWebsocketHandler(const char* url,websocket_api_handler f,const char* file,int line){
	register_websocket_handler(url,f,file,line);
}


void WebserverAddFileDir(const char* alias,const char* dir){
	setFileDir(alias,dir);
}

void WebserverAddFileDirNoCache(const char* alias,const char* dir){
	setFileDirNoCache(alias,dir);
}


void WebserverAddTemplateFilePostfix(const char* postfix){
	addTemplateFilePostfix(postfix);
}

void WebserverAddTemplateIgnoreFilePostfix(const char* postfix){
	addTemplateIgnoreFilePostfix(postfix);
}


int WebserverInit(void){
	if(initWebserver() != 0){
		//asm (".symver WebserverShutdownHandler1__,WebserverShutdownHandler@@VERS_1.0");
		return 0;
	}
	return -1;
}

void WebserverStart(void){
	startWebServer();
}

void WebserverShutdown(void){
	shutdownWebserver();
}

void WebserverShutdownHandler(void){
	breakEvents();
	//shutdownWebserverHandler();
}
//#include <asm.h>



void WebserverConfigSetInt(const char* name,int value){
	setConfigInt(name,value);
}

int  WebserverConfigGetInt(const char* name){
	return getConfigInt(name);
}

void WebserverConfigSetText(const char* name, const char* text){
	setConfigText(name,text);
}

void WebserverRegisterPluginErrorHandler(plugin_error_handler f){
	RegisterPluginErrorHandler(f);
}

void WebserverInjectExternFD(int fd, extern_handler handle ){
	socket_info* sock = WebserverMallocSocketInfo();
	sock->extern_handle = handle;
	sock->socket = fd;
	addEventSocketReadPersist(sock);
}


#pragma GCC visibility pop
