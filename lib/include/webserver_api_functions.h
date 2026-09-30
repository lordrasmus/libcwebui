/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

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
#define NORMAL_CHECK_OK			( 1 )
#define SSL_CHECK_OK			( 2 )
#define NOT_REGISTERED			( 0 )
#define SESSION_MISMATCH_ERROR 	( -1 )

/* Session Status */
#define SESSION_NOT_REGISTERED 			( 0 )
#define SESSION_REGISTERED			    ( 1 )


/* Session Store Typen */
#define STORE_NORMAL			( 1 )
#define STORE_SSL			    ( 2 )

/* Condition Rückgabewerte */
#define COND_TRUE			    ( 1 )
#define COND_FALSE		     	( 2 )
#define COND_ERROR		     	( 3 )

/* Websocket Signale */
#define WEBSOCKET_CONNECT 		( 1 )
#define WEBSOCKET_MSG 			( 2 )
#define WEBSOCKET_DISCONNECT 		( 3 )



#define WEBSERVER_API 6

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
#ifdef __GNUC__
	void printHTML(dummy_handler* s,const char *fmt,...) __attribute__((format(printf, 2, 3)));
#else
	void printHTML(dummy_handler* s, const char *fmt, ...);
#endif

/*
		Buffer in den Ausgabe Buffer kopieren
*/
void sendHTML(dummy_handler* s, const char* text, const unsigned int length);

/*
		Custom Response Header setzen (für WS_FILE_TYPE_CUSTOM)
		Muss vor sendHTML() aufgerufen werden.
*/
void ws_set_response_header(dummy_handler* s, const char* name, const char* value);


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

char* 	WebsocketGetStoreGUID(char* guid);
long 	ws_get_websocket_store_timeout( char* guid );

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


dummy_var* 	ws_get_render_var(dummy_handler* s,const char* name,WS_VAR_FLAGS flags);

void 		ws_set_render_var(dummy_handler* s,char* name,char* text);
#ifdef __GNUC__
	void 		setRenderVar_v (dummy_handler* s, char* name, char* format,...) __attribute__((format(printf, 3, 4)));
#else
	void 		setRenderVar_v(dummy_handler* s, char* name, char* format, ...);
#endif




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

#define NO_CHECK_NAME_EXISTS 1

dummy_var* addToVarArray(dummy_var* var, const char* name);
dummy_var* addToVarArray2(dummy_var* var, const char* name, uint32_t flags);
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
int64_t  getVariableAsLong(dummy_var* var);
/*
		Variablen Inhalt als Int schreiben
*/
void setVariableAsInt(dummy_var* var, int value);
void setVariableAsULong(dummy_var* var, uint64_t value);
void setVariableAsLong(dummy_var* var, int64_t value);

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
/*
 * Die hochgeladenen Dateien eines multipart/form-data Requests, in der
 * Reihenfolge des Bodys. Formularfelder ohne filename= stehen hier nicht -
 * ein Request ohne Datei liefert an Index 0 also 0 und nicht etwa den Namen
 * des ersten Feldes.
 *
 * Bei gestreamten Uploads liegen die Daten nicht im Speicher: getFileData()
 * liefert dann 0, der Inhalt ist bereits ueber on_chunk() beim Handler
 * gelandet. getFileName() und getFileSize() gelten weiterhin.
 */
char*  getFileName(dummy_handler* s,int index);
char*  getFileData(dummy_handler* s,int index);
int    getFileSize(dummy_handler* s,int index);
char*  getPostData(dummy_handler* s);
int    getPostSize(dummy_handler* s);

/* Client-IP der Verbindung als String ("" wenn nicht ermittelbar). */
const char* ws_get_client_ip(dummy_handler* s);


/***********************************************************************************************
*                                                                                              *
*          Request Status Informationen                                                        *
*                                                                                              *
************************************************************************************************/

/*
      URL des Requests
*/
const char *getRequestURL(dummy_handler* s);

/*
      Host des Requests
*/
const char *getRequestHost(dummy_handler* s);

/*
      Ob der Request über eine SSL Verbindung kam
*/
char isRequestSecure(dummy_handler *s);

/*
      GET Parameter des Requests abfragen
*/
dummy_var* getURLParameter(dummy_handler* s,const char* name);

/*
      HTTP Header Zeilen abfragen
*/
const char* getRequestHeader(dummy_handler *s, char* name);

/*
      Parameter die in der Templateengine mit {f:<name>:para1:para2:...} uebergeben wurden
*/
char* getEngineParameter(dummy_handler* s,int index);
void  dumpEngineParameter(dummy_handler* s);


/*
	String auf UTF-8 testen
*/
int ws_check_utf8( unsigned char *str, uint32_t len, char **message);

/*
	decode URL String 
*/
void ws_url_decode(char *line);

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

      use_cache = files im ram cachen
      use_auth  = nur mit authentifizierung sichtbar
*/
void ws_add_dir(const char* alias,const char* dir, const int use_cache, const int use_auth );



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

/***********************************************************************************************
*                                                                                              *
*          WebSocket Streaming API                                                             *
*                                                                                              *
************************************************************************************************/

#ifdef WEBSERVER_USE_WEBSOCKETS

/**
 * Streaming context for large WebSocket frames.
 * Passed to all streaming callbacks.
 */
typedef struct websocket_stream_context {
    char* guid;                    /* WebSocket Connection GUID */
    char* url;                     /* Request URL */
    uint64_t total_length;         /* Total frame length (from header) */
    uint64_t received_length;      /* Bytes received so far */
    char is_binary;                /* Binary or Text frame */
    char aborted;                  /* Handler aborted the stream */
    void* user_data;               /* Handler-specific data */
} websocket_stream_context;

/**
 * Streaming callback types
 */
typedef void (*ws_stream_start_handler)(websocket_stream_context* ctx);
typedef void (*ws_stream_chunk_handler)(websocket_stream_context* ctx,
                                         const unsigned char* data,
                                         uint32_t length);
typedef void (*ws_stream_end_handler)(websocket_stream_context* ctx,
                                       int success);  /* success=0 on error/abort */

/**
 * Register a streaming handler for a WebSocket URL.
 * When registered, ALL frames on this URL use streaming (no threshold).
 */
void RegisterWebsocketStreamHandler(
    const char* url,
    ws_stream_start_handler on_start,
    ws_stream_chunk_handler on_chunk,
    ws_stream_end_handler on_end
);

/**
 * Helper functions for streaming handlers
 */
void WebsocketStreamSetUserData(websocket_stream_context* ctx, void* data);
void* WebsocketStreamGetUserData(websocket_stream_context* ctx);

/**
 * Abort the current stream (e.g., on handler error)
 */
void WebsocketStreamAbort(websocket_stream_context* ctx);

#endif /* WEBSERVER_USE_WEBSOCKETS */

/***********************************************************************************************
*                                                                                              *
*          POST Streaming API                                                                  *
*                                                                                              *
************************************************************************************************/

/**
 * Streaming context for large multipart/form-data POST uploads.
 * Passed to all streaming callbacks.
 *
 * Ohne Streaming wird der komplette POST Body in den RAM geladen und der
 * Datei Part daraus noch einmal kopiert. Der Spitzenbedarf ist dann das
 * Doppelte der Dateigroesse. Mit einem registrierten Streaming Handler
 * wird der Body nie gepuffert, der Bedarf ist konstant.
 */
typedef struct post_stream_context {
	char*    url;               /* Request URL                                  */
	char*    field_name;        /* name="..."     aus Content-Disposition        */
	/* filename="..." aus Content-Disposition. Steht so im Request wie der
	 * Client ihn geschickt hat - er kann Pfadanteile enthalten ( "../.." ).
	 * Wer damit eine Datei anlegt, muss ihn vorher selbst bereinigen. */
	char*    file_name;
	uint64_t total_length;      /* Content-Length der gesamten Anfrage           */
	uint64_t received_length;   /* Nutzbytes dieses Parts bisher                 */
	char     aborted;           /* Handler hat den Stream abgebrochen            */
	void*    user_data;         /* Handler spezifische Daten                     */
} post_stream_context;

/**
 * Streaming callback types
 */
typedef void (*post_stream_start_handler)(post_stream_context* ctx);
typedef void (*post_stream_chunk_handler)(post_stream_context* ctx,
                                          const unsigned char* data,
                                          uint32_t length);
typedef void (*post_stream_end_handler)(post_stream_context* ctx,
                                        int success);  /* success=0 on error/abort */

/**
 * Register a streaming handler for a POST URL.
 * When registered, ALL multipart/form-data uploads on this URL use streaming
 * (no threshold). Andere Content-Types der URL laufen unveraendert ueber den
 * normalen POST Buffer.
 *
 * on_start / on_end werden pro Datei Part gerufen, also einmal pro Datei.
 * Parts ohne filename= sind Formularfelder, die weiterhin normal gepuffert
 * und ueber getFileName()/getFileData() bereitgestellt werden.
 *
 * Ein fuehrender Slash in der URL ist optional, "/a/b.html" und "a/b.html"
 * sind gleichwertig.
 *
 * auth_check laeuft, bevor der Body gelesen wird, und entscheidet ob dieser
 * Request schreiben darf. Liefert sie 0, werden die Callbacks nicht gerufen
 * und nichts gepuffert oder geschrieben - der Body wird aber weiterhin vom
 * Socket gelesen und verworfen. 0 als Zeiger heisst: keine Pruefung, jeder
 * darf.
 *
 * Das Verwerfen ist bewusst so: der Request laeuft danach normal weiter zum
 * Handler, damit eine REST URL ihre gewohnte JSON Antwort liefert und nicht
 * eine Fehlerseite, mit der das Frontend nichts anfangen kann. Der Preis ist,
 * dass ein Unberechtigter weiterhin Bandbreite und Parse Zeit kosten kann -
 * bis max_size Bytes. Gegen Speicher und Dateisystem schuetzt es, gegen
 * Netzlast nicht.
 *
 * Die Pruefung gehoert dem Consumer, nicht dem Webserver: nur er weiss,
 * welches Recht sein Upload verlangt. Ein Firmware Upload, der schon in
 * on_start() die Zieldatei anlegt, muss hier ACL_WRITE verlangen - sonst
 * koennte ein angemeldeter Benutzer ohne Schreibrecht die vorhandene Datei
 * loeschen lassen, bevor der URL Handler ueberhaupt laeuft.
 *
 * Nur beim Start registrieren ( init_webserver_plugin ). Die Registry wird
 * beim Request ohne Lock gelesen, eine Registrierung zur Laufzeit waere ein
 * Race gegen laufende Verbindungen.
 *
 * max_size begrenzt den Body dieser URL. 0 bedeutet: globales max_post_size
 * verwenden. Damit kann max_post_size klein bleiben - gross genug fuer
 * Formulare und Zertifikate - und nur die URL, die tatsaechlich eine Firmware
 * entgegennimmt, erlaubt deren Groesse. Sonst koennte an jeder beliebigen URL
 * ein Body in Firmware-Groesse abgeladen werden.
 */
void RegisterPostStreamHandler(
	const char* url,
	post_stream_auth_handler auth_check,
	uint64_t max_size,
	post_stream_start_handler on_start,
	post_stream_chunk_handler on_chunk,
	post_stream_end_handler   on_end
);

/**
 * Helper functions for streaming handlers
 */
/**
 * Groesstes Limit ueber alle registrierten Streaming Handler ( 0 = keiner ).
 * Fuer die Frage "wie gross darf ein Upload hoechstens sein", die ein
 * Frontend vor dem Absenden stellt.
 */
uint64_t GetMaxPostStreamSize(void);

void  PostStreamSetUserData(post_stream_context* ctx, void* data);
void* PostStreamGetUserData(post_stream_context* ctx);

/**
 * Abort the current stream (e.g., on handler error).
 *
 * Der Abbruch gilt fuer den restlichen Request, nicht nur fuer den laufenden
 * Part:
 *
 *  - Der laufende Part bekommt keine weiteren on_chunk() mehr, aber noch
 *    on_end() mit success == 0.
 *  - Fuer alle folgenden Parts laeuft gar kein Callback mehr an - weder
 *    on_start() noch on_chunk() noch on_end().
 *  - Der restliche Body wird noch vom Socket gelesen und verworfen, sonst
 *    haengt die Verbindung.
 *  - Nichts davon erscheint in getFileName()/getFileSize(): weder die
 *    abgebrochene Datei noch Formularfelder, die nach dem Abbruch folgen.
 *    Was vor dem Abbruch vollstaendig angekommen ist, bleibt dagegen
 *    erhalten - der URL Handler sieht die Felder und Dateien bis dahin.
 *
 * Unabhaengig davon gilt fuer Formularfelder eine Groessengrenze ( je Feld und
 * in der Summe ). Ein Feld, das sie reisst, wird nicht gekuerzt weitergegeben
 * sondern ganz weggelassen: ein gekuerzter Wert waere von einem echt so
 * gesendeten nicht zu unterscheiden. Der Handler sieht das Feld also gar
 * nicht, der Rest des Requests bleibt gueltig.
 *
 * Der Aufruf ist auch in on_end() noch erlaubt - dort faellt zum Beispiel
 * eine Pruefsumme ueber die fertige Datei auf. Die Datei wird dann ebenfalls
 * nicht mehr als vorhanden gemeldet.
 *
 * Denselben Zustand loest die interne Obergrenze fuer die Anzahl der Parts
 * eines Requests aus ( POST_STREAM_MAX_PARTS ): alles ab dem Limit wird
 * verworfen. Der Request selbst laeuft danach normal in den URL Handler, der
 * dann nur die Parts bis zum Limit sieht.
 */
void PostStreamAbort(post_stream_context* ctx);

void WebserverRegisterPluginErrorHandler(plugin_error_handler f);


int  WebserverInit(void);
void WebserverStart(void);
void WebserverShutdown(void);
void WebserverShutdownHandler(void);
int  WebserverSSLReload(void);

void WebserverConfigSetInt(const char* name, int value);
int  WebserverConfigGetInt(const char* name);
void WebserverConfigSetText(const char* name, const char* text);
void WebserverConfigGetText(const char* name, char* text, int text_size);

/*
 * Hand a file descriptor to the event loop and have `handle` called on it.
 *
 * On an SSL connection the handler is expected to consume what the SSL layer
 * has already decrypted. Those bytes are no longer in the kernel buffer, so
 * epoll will not announce them a second time - a handler that returns while
 * leaving them behind gets no further event for them.
 *
 * A handler that deliberately stops reading (backpressure, e.g. no room left
 * to forward the data) is therefore responsible for getting itself called
 * again once it can accept data. The event loop detects the stall and stops
 * calling it, so the connection does not burn CPU, but it cannot supply the
 * wakeup.
 */
void WebserverInjectExternFD(int fd, extern_handler handle );


void WebserverSetPostHandler( post_handler handler );


void ws_generate_guid(char* buf, int length);

typedef enum {
	WS_FILE_TYPE_PLAIN,
	WS_FILE_TYPE_HTML,
	WS_FILE_TYPE_CSS,
	WS_FILE_TYPE_JS,
	WS_FILE_TYPE_XML,
	WS_FILE_TYPE_XSL,
	WS_FILE_TYPE_SVG,
	WS_FILE_TYPE_JSON,
	WS_FILE_TYPE_CUSTOM   /* Custom headers set via setResponseHeader() */

} WS_FILE_TYPES;

typedef WS_FILE_TYPES (*url_handler_func)( dummy_handler* s, const char* url );
void ws_register_url_function( char* url, url_handler_func func );

void ws_reverse_proxy_register_uds( const char* method, const char* url, const char* path );


/*
 *		Python API
 *
 */


void WebserverInitPython( void );
int WebserverLoadPyPlugin( const char* path );


/*
 *		CORS API
 */

void  ws_set_cors_handler( cors_handler handler );
char* ws_get_cors_type_name( CORS_HEADER_TYPES type );

#include "webserver_api_functions_depricated.h"


#ifdef __cplusplus
}
#endif

#endif
