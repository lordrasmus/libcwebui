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

#ifndef _WEBSERVER_API_FUNCTIONS_
#define _WEBSERVER_API_FUNCTIONS_


#include "WebserverConfig.h"
#include "webserver_api_functions_macros.h"

#if __GNUC__ > 2
	#define NEED_RESUL_CHECK __attribute__((warn_unused_result))
#else
	#define NEED_RESUL_CHECK
#endif



/* Authentifizierung über Cookies */
#define NORMAL_CHECK_OK			1
#define SSL_CHECK_OK			2
#define NOT_REGISTERED			0
#define SESSION_MISMATCH_ERROR -1

/* Session Status */
#define NOT_REGISTERED 	0
#define REGISTERED 		1


/* Session Store Typen */
#define STORE_NORMAL  1
#define STORE_SSL	  2

/* Condition Rückgabewerte */
#define COND_TRUE 		1
#define COND_FALSE 		2
#define COND_ERROR  	3

/* Websocket Signale */
#define WEBSOCKET_CONNECT 		1
#define WEBSOCKET_MSG 			2
#define WEBSOCKET_DISCONNECT 	3



#define WEBSERVER_API 5

#ifdef __cplusplus
extern "C" {
#endif


#ifndef WebserverMalloc


	#ifdef _WEBSERVER_MEMORY_DEBUG_
		void*	real_WebserverMalloc( const unsigned long size, const char* function, const char* filename, int fileline);
		#define WebserverMalloc(a) real_WebserverMalloc(a,__FUNCTION__,(char*)__BASE_FILE__, __LINE__)
	#else
		void*   real_WebserverMalloc( const unsigned long size );
		#define WebserverMalloc(a) real_WebserverMalloc(a)
	#endif

	void	WebserverFree( void *mem );

#endif


void WebserverGenerateGUID(char* buf, int length);


/***********************************************************************************************
*                                                                                              *
*           Ausgabe Funktionen                                                                 *
*                                                                                              *
************************************************************************************************/

/*
 		Variable in den Ausgabe Buffer schreiben
*/
void sendHTMLVariable(dummy_handler* s,dummy_var* var);

/*
		printf Funktion die in der Ausgabe Buffer schreibt
*/
void printHTML(dummy_handler* s,const char *fmt,...) __attribute__ ((format (printf, 2, 3)));

/*
		Buffer in den Ausgabe Buffer kopieren
*/
void sendHTML(dummy_handler* s, const char* text, const unsigned int length);

/*
 		printf Funktion die in den firelogger Buffer schreibt

 	https://github.com/darwin/firelogger/wiki
	 https://addons.mozilla.org/de/firefox/addon/firelogger/
*/
#define FireLoggger( ARGS... ) addFireLogger(s,(char*)__BASE_FILE__, __LINE__, ARGS)
void addFireLogger(dummy_handler* s, const char* filename, int fileline, const char* fmt, ... ) __attribute__ ((format (printf, 4, 5)));

/*
		Globalen Variablen locken / unlocken
*/
int NEED_RESUL_CHECK lockGlobalVars(void);
int NEED_RESUL_CHECK unlockGlobalVars(void);


/***********************************************************************************************
*                                                                                              *
*           Variablen verwalten                                                                *
*                                                                                              *
************************************************************************************************/

typedef enum {
	NO_FLAGS      = 0x00,
	DO_NOT_CREATE = 0x01
}WS_VAR_FLAGS;

char* WebsocketGetStoreGUID(char* guid);

char setSessionVarGUID(char* store_guid, int store, const char* name, const char* value);

dummy_var* getSessionVarGUID(char* store_guid, int store, const char* name,WS_VAR_FLAGS flags);

char* getSessionGUID(dummy_handler* s);

/*
		Session Variable setzen

		store = STORE_NORMAL ] STORE_SSL
*/
char setSessionVar(dummy_handler* s,int store,const char* name,const char* value);

/*
		Session Variable holen

		store = STORE_NORMAL ] STORE_SSL
*/
dummy_var* getSessionVar(dummy_handler* s,int store,const char* name,WS_VAR_FLAGS flags);


/*
		Render Variable setzen
*/
void setRenderVar   (dummy_handler* s,char* name,char* text);
void setRenderVar_v (dummy_handler* s, char* name, char* format,...) __attribute__ ((format (printf, 3, 4)));


/*
		Render Variable holen
*/
dummy_var* getRenderVar(dummy_handler* s,const char* name,WS_VAR_FLAGS flags);


/*
		Glole Variable setzen
*/
dummy_var* setGlobalVar(const char* name,const char* text);


/*
		Glole Variable holen
*/
dummy_var* getGlobalVar(const char* name,WS_VAR_FLAGS flags);


/***********************************************************************************************
*                                                                                              *
*           Variablen lesen / schreiben                                                        *
*                                                                                              *
************************************************************************************************/

/*
		Variable zu einem Array machen
*/
void setVariableToArray(dummy_var* var);

/*
		Neue Variable in einem Array erzeugen
*/
dummy_var* addToVarArray(dummy_var* var, const char* name);
dummy_var* addToVarArrayCustomData(dummy_var* var, const char* name,free_handler handle, void* data );

void addToVarArrayInt(dummy_var* var, const char* name, int value );
void addToVarArrayStr(dummy_var* var, const char* name, char* buffer );

void* getVariableAsCustomData(dummy_var* var);

/*
		Variablen Inhalt als String lesen
*/
int   getVariableAsString(dummy_var* var, char* buffer,unsigned int buffer_length);

/*
		Internen Variablen String Pointer lese

      ACHTUNG : Pointer nicht speichern da die Variablen vom webserver gelöscht werden können
*/
char* getVariableAsStringP(dummy_var* var);

/*
		Variablen Inhalt als String schreiben
*/
void setVariableAsString(dummy_var* var, const char* text);

/*
		Variablen Inhalt als Int lesen
*/
int getVariableAsInt(dummy_var* var);
uint64_t getVariableAsULong(dummy_var* var);

/*
		Variablen Inhalt als Int schreiben
*/
void setVariableAsInt(dummy_var* var, int value);
void setVariableAsULong(dummy_var* var, uint64_t value);

void setVariableToRef(dummy_var* var,dummy_var* ref);

dummy_var*	getArrayVariable(dummy_var* var, const char* name);

dummy_var*	getArrayFirst(dummy_var* var);
dummy_var*	getArrayNext(dummy_var* var);
void 		stopArrayIterate(dummy_var* var);


/***********************************************************************************************
*                                                                                              *
*          Session Authentifizierung                                                           *
*                                                                                              *
************************************************************************************************/

/*
		Session Registrierungs Status einstellen

		status = REGISTERED  |  NOT_REGISTERED
*/
char setUserRegisterStatus(dummy_handler* s,char status);

/*
      Session Register Status abrufen

      ret = NORMAL_CHECK_OK | SSL_CHECK_OK | NOT_REGISTERED | SESSION_MISMATCH_ERROR
*/
int checkUserRegisterStatus(dummy_handler* s);

/***********************************************************************************************
*                                                                                              *
*          POST File Upload                                                                    *
*                                                                                              *
************************************************************************************************/

int    getFileCount(dummy_handler* s);
char*  getFileName(dummy_handler* s,int index);
char*  getFileData(dummy_handler* s,int index);
int    getFileSize(dummy_handler* s,int index);
char*  getPostData(dummy_handler* s);


/***********************************************************************************************
*                                                                                              *
*          Request Status Informationen                                                        *
*                                                                                              *
************************************************************************************************/

/*
      URL des Requests
*/
char *getRequestURL(dummy_handler* s);

/*
      Ob der Request über eine SSL Verbindung kam
*/
char isRequestSecure(dummy_handler *s);

/*
      GET Parameter des Requests abfragen
*/
dummy_var* getURLParameter(dummy_handler* s,const char* name);

/*
      Parameter die in der Templateengine mit {f:<name>:para1:para2:...} uebergeben wurden
*/
char* getEngineParameter(dummy_handler* s,int index);
void  dumpEngineParameter(dummy_handler* s);

/*
      Text Frame über einen Websocket senden
*/
int WebsocketSendTextFrame(const char* guid, const char* in, const int length);
int WebsocketSendBinaryFrame(const char* guid, const char* in, const int length);
/*
      Close Frame über einen Websocket senden
*/
int WebsocketSendCloseFrame(const char* guid);

/*
      Dir als /<alias>/ im Webserver einblenden
*/
void WebserverAddFileDir(const char* alias,const char* dir);
void WebserverAddFileDirNoCache(const char* alias,const char* dir);

void WebserverAddBinaryData(const unsigned char* data);

/*
		Datei <path> als Webserver Plugin laden
*/
int WebserverLoadPlugin(const char* name,const char* path);


/*

		Regeln um einzustellen was als Template behandelt werden soll
*/

void WebserverAddTemplateFilePostfix(const char* postfix);
void WebserverAddTemplateIgnoreFilePostfix(const char* postfix);


/* Urls die nicht im RAM gecached werden soll */
void WebserverAddNoRamCacheFile( const char* url );

/* Urls die nicht sichtbar sein sellen */
void WebserverAddBlockedFile( const char* url );

void RegisterEngineFunction(const char* name,user_api_function f,const char* file,int line);
void RegisterEngineCondition(const char* name,user_api_condition f,const char* file,int line);
void RegisterWebsocketHandler(const char* url,websocket_api_handler f,const char* file,int line);

void WebserverRegisterPluginErrorHandler(plugin_error_handler f);


int  WebserverInit(void);
void WebserverStart(void);
void WebserverShutdown(void);
void WebserverShutdownHandler(void);

void WebserverConfigSetInt(const char* name, int value);
int  WebserverConfigGetInt(const char* name);
void WebserverConfigSetText(const char* name, const char* text);

void WebserverInjectExternFD(int fd, extern_handler handle );


void WebserverSetPostHandler( post_handler handler );


/*
 *		Python API
 *
 */


void WebserverInitPython( void );
int WebserverLoadPyPlugin( const char* path );




#ifdef __cplusplus
}
#endif

#endif
