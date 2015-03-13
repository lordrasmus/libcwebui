/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/



#ifndef _WEBSERVER_API_FUNCTIONS_MACROS_
#define _WEBSERVER_API_FUNCTIONS_MACROS_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


/***********************************************************************************************
*                                                                                              *
*           Plugin Macros                                                                      *
*                                                                                              *
*           Initialisierung von Plugins                                                      *
*           Definition und Registrierung von Plugin Funktionen                                 *
*                                                                                              *
*                                                                                              *
************************************************************************************************/




/************************************************************************************************
*                                                                                               *
* 			Macro für die initialisierung eines Plugins                                         *
*                                                                                               *
*			die erzeugten Funtionen werden direkt mit dlsym geladen und kollidieren             *
*			daher nicht wenn mehrer Plugins geladen werden                                      *
*                                                                                               *
************************************************************************************************/

#if __GNUC__ > 2
        #define VISIBLE_ATTR __attribute__ ((visibility("default")))
#else
        #define VISIBLE_ATTR
#endif

#define WEBSERVER_API_HOOK \
		int get_webserver_api_version(void) VISIBLE_ATTR ; \
		int get_webserver_api_version(void)	{ \
			return WEBSERVER_API;\
		} \
		char* init_webserver_plugin(void) VISIBLE_ATTR ; \
		char* init_webserver_plugin(void)


/***********************************************************************************************
*                                                                                              *
*           Macros für die Definition und Registrierung von Plugin Funktionen                  *
*                                                                                              *
************************************************************************************************/

#if __GNUC__ > 2

	#define DEFINE_FUNCTION( a ) \
		const char*			ws_ef_##a##_df = __FILE__; \
		const int			ws_ef_##a##_dl = __LINE__; 	\
		void 				ws_ef_##a ( dummy_handler *s,dummy_func* func ) ; \
		void 				ws_ef_##a ( dummy_handler *s,dummy_func* func )

	#define REGISTER_FUNCTION( a )	{ \
		extern const char*	ws_ef_##a##_df; \
		extern const int 	ws_ef_##a##_dl; \
		extern void 		ws_ef_##a ( dummy_handler *s,dummy_func* func ) ; \
		RegisterEngineFunction ( #a,ws_ef_##a,ws_ef_##a##_df,ws_ef_##a##_dl ); \
		}

	#define REGISTER_LOCAL_FUNCTION( a ) 	RegisterEngineFunction ( #a,ws_ef_##a,ws_ef_##a##_df,ws_ef_##a##_dl );

#else
	#define DEFINE_FUNCTION( a ) void 			  ws_ef_##a ( dummy_handler *s,dummy_func* func )  ; void ws_ef_##a ( dummy_handler *s,dummy_func* func )
	#define REGISTER_FUNCTION( a )	{ extern void ws_ef_##a ( dummy_handler *s,dummy_func* func )  ; RegisterEngineFunction ( #a,ws_ef_##a,"",0 ); 	}
	#define REGISTER_LOCAL_FUNCTION( a ) 	RegisterEngineFunction ( #a,ws_ef_##a,"",0 );
#endif





/***********************************************************************************************
*                                                                                              *
*           Macros für die Definition und Registrierung von Plugin Conditions                  *
*                                                                                              *
************************************************************************************************/


#define DEFINE_CONDITION( a )	\
		const char*			ws_ec_##a##_df = __FILE__; \
		const int          	ws_ec_##a##_dl = __LINE__; \
		int                	ws_ec_##a ( dummy_handler *s,dummy_func* func ) ; \
		int                	ws_ec_##a ( dummy_handler *s,dummy_func* func )

#define REGISTER_CONDITION( a )	{ \
		extern const char*	ws_ec_##a##_df; \
		extern const int   	ws_ec_##a##_dl ; \
		extern int         	ws_ec_##a ( dummy_handler *s,dummy_func* func ) ; \
		RegisterEngineCondition ( #a,ws_ec_##a,ws_ec_##a##_df,ws_ec_##a##_dl ); \
	}

#define REGISTER_LOCAL_CONDITION( a ) \
		RegisterEngineCondition ( #a,ws_ec_##a,ws_ec_##a##_df,ws_ec_##a##_dl );



/***********************************************************************************************
*                                                                                              *
*           Macros für die Definition und Registrierung von Plugin Websocket Handlern          *
*                                                                                              *
************************************************************************************************/

#define DEFINE_WEBSOCKET_HANDLER( a, b )\
		const char*			ws_wh_##b##_df_url = a; \
		const char*			ws_wh_##b##_df = __FILE__; \
		const int 			ws_wh_##b##_dl = __LINE__; \
		void 				ws_wh_##b ( const int signal, const char* guid, const char binary, const char* msg, const unsigned long long len ); \
		void 				ws_wh_##b ( const int signal, const char* guid, const char binary, const char* msg, const unsigned long long len )

#define REGISTER_WEBSOCKET_HANDLER( b  )	{ \
		extern const char*	ws_wh_##b##_df_url; \
		extern const char*	ws_wh_##b##_df; \
		extern const int 	ws_wh_##b##_dl; \
		extern void 		ws_wh_##b ( const int signal, const char* guid, const char binary, const char* msg, const unsigned long long len ); \
		RegisterWebsocketHandler ( ws_wh_##b##_df_url, ws_wh_##b, ws_wh_##b##_df,ws_wh_##b##_dl ); \
	}

#define REGISTER_LOCAL_WEBSOCKET_HANDLER( b ) \
		RegisterWebsocketHandler ( ws_wh_##b##_df_url, ws_wh_##b, ws_wh_##b##_df,ws_wh_##b##_dl );



/***********************************************************************************************
*                                                                                              *
*           Typedefinitionen und Prototypen für Plugin Funktionen                             *
*                                                                                              *
************************************************************************************************/

typedef struct{	void *p;} dummy_handler;
typedef struct{	void *p;} dummy_func;
typedef struct{	union { void *p; uint64_t t;} dummy_u ; } dummy_var;


typedef void ( *free_handler ) ( void* ptr);
#ifndef _DATATYPES_H_
	typedef void ( *extern_handler ) ( int fd , void* ptr);
#endif
typedef void ( *user_api_function ) ( dummy_handler *s,dummy_func* func );
typedef int ( *user_api_condition ) ( dummy_handler* s,dummy_func* func );
typedef void ( *websocket_api_handler ) ( const int signal, const char* guid, const char binary, const char* msg, const unsigned long long len );

typedef void ( *post_handler ) ( char* status, char* url );

typedef enum {
	PLUGIN_LOADING,
	PLUGIN_LOADED,
	PLUGIN_CHECK_ERROR,
	PLUGIN_RESET_CRASHED,
	PLUGIN_FUNCTION_CALLING,
	PLUGIN_FUNCTION_CALLED,
	PLUGIN_FUNCTION_CHECK_ERROR,
	PLUGIN_FUNCTION_RESET_CRASHED,
}PLUGIN_ERROR_TYPES;

typedef int ( *plugin_error_handler ) ( PLUGIN_ERROR_TYPES error,const char* plugin,const char* function,const char* text );



/***********************************************************************************************
*                                                                                              *
*           Typedefinitionen und Prototypen für CORS Handler                             *
*                                                                                              *
************************************************************************************************/

typedef enum {
	CORS_ALLOW_ORIGIN,
	CORS_ALLOW_CREDENTIALS,
	CORS_ALLOW_METHODS,
	CORS_ALLOW_HEADERS
}CORS_HEADER_TYPES;

typedef struct{
	CORS_HEADER_TYPES type;
	char* method;
	char* origin;
}cors_infos;

typedef int ( *cors_handler ) ( cors_infos* infos );


#ifdef __cplusplus
}
#endif

#endif
