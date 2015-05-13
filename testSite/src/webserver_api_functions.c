

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "webserver_api_functions.h"


#define OUT_BUFFER_SIZE 10000

void startApiThreads(void);



DEFINE_FUNCTION ( checkregister ) {
    int i;

    i = checkUserRegisterStatus ( s );
    switch ( i ) {
        case NORMAL_CHECK_OK		:
        	printHTML( s , "NORMAL CHECK OK" );
			return;
        case SSL_CHECK_OK		:
        	printHTML( s , "SSL CHECK OK" );
			return;
        case NOT_REGISTERED		:
        	printHTML( s , "NOT REGISTERED" );
			return;
        case SESSION_MISMATCH_ERROR 	:
        	printHTML( s , "SESSION MISMATCH ERROR" );
			return;
        default			:
        	printHTML( s , "checkUserRegistered unknown return value : %d",i );
			return;
	}
}

DEFINE_FUNCTION ( register ) {
	setUserRegisterStatus ( s,1 );
}

DEFINE_FUNCTION ( unregister ) {
	setUserRegisterStatus ( s,0 );
}

DEFINE_FUNCTION ( setrvvar ) {

	dummy_var *var;
	dummy_var *var_array;

    setGlobalVar("g_test1","G_t1");
    setGlobalVar("g_test2","G_t2");
    setGlobalVar("g_test3","G_t3");

	var = getRenderVar ( s, ( char* ) "test1", NO_FLAGS);
	setVariableAsString(var,"test var 1");

	var = getRenderVar ( s, ( char* ) "test2", NO_FLAGS);
	setVariableAsString(var,"test var 2");

	var = getRenderVar ( s, ( char* ) "test3", NO_FLAGS);
	setVariableAsString(var,"test var 3");

	var_array = getRenderVar ( s, ( char* ) "testarray1", NO_FLAGS);
	setVariableToArray(var_array);
	var = addToVarArray(var_array,"entry_2");
	setVariableAsString(var,"entry_2_text");
	var = addToVarArray(var_array,"entry_3");
	setVariableAsString(var,"entry_3_text");
	var = addToVarArray(var_array,"entry_1");
	setVariableAsString(var,"entry_1_text");


}

DEFINE_FUNCTION ( cachemenustart ) {
    char buffer[1000];
    char buffer2[1000];
    dummy_var *var;

    setRenderVar ( s, ( char* ) "ctmp1_class", ( char* ) "inaktive" );
    setRenderVar ( s, ( char* ) "ctmp2_class", ( char* ) "inaktive" );
    setRenderVar ( s, ( char* ) "ctmp3_class", ( char* ) "inaktive" );
    setRenderVar ( s, ( char* ) "ctmp4_class", ( char* ) "inaktive" );
    setRenderVar ( s, ( char* ) "ctmp5_class", ( char* ) "inaktive" );
	setRenderVar ( s, ( char* ) "ctmp6_class", ( char* ) "inaktive" );
    
    setRenderVar ( s, ( char* ) "ctmp11_class", ( char* ) "inaktive" );
    setRenderVar ( s, ( char* ) "ctmp22_class", ( char* ) "inaktive" );
    setRenderVar ( s, ( char* ) "ctmp33_class", ( char* ) "inaktive" );
    setRenderVar ( s, ( char* ) "ctmp44_class", ( char* ) "inaktive" );
    setRenderVar ( s, ( char* ) "ctmp55_class", ( char* ) "inaktive" );
	setRenderVar ( s, ( char* ) "ctmp66_class", ( char* ) "inaktive" );

    var = getRenderVar ( s, ( char* ) "cache_menu", NO_FLAGS);
    if ( var != 0 ) {
		getVariableAsString(var,buffer,1000);
        if ( isRequestSecure(s) == 1 ) {
            snprintf ( buffer2,1000, ( char* ) "ctmp%s%s_class",buffer,buffer );
            setRenderVar ( s,buffer2, ( char* ) "aktive" );
        }
        snprintf ( buffer2,1000, ( char* ) "ctmp%s_class",buffer );
        setRenderVar ( s,buffer2, ( char* ) "aktive" );
    }

}


DEFINE_FUNCTION ( sessionmenustart ) {
    int i;
    char buffer[1000];
    dummy_var *var = getURLParameter ( s, ( char* ) "testpara2" );
    if ( var!=0 ) {
		i = getVariableAsInt(var);
        if ( isRequestSecure(s) == 1 )
            i+=4;
        snprintf ( buffer,1000,"%d",i );
        setRenderVar ( s, ( char* ) "session_menu",buffer );
    } else {
        if ( isRequestSecure(s) == 1 )
            setRenderVar ( s, ( char* ) "session_menu", ( char* ) "4" );
        else
            setRenderVar ( s, ( char* ) "session_menu", ( char* ) "0" );
    }
}

DEFINE_FUNCTION ( menustart ) {

    FireLoggger (  "menustart" );
    setRenderVar ( s, ( char* ) "mp1_class", ( char* ) "inaktive" );
    setRenderVar ( s, ( char* ) "mp2_class", ( char* ) "inaktive" );
    setRenderVar ( s, ( char* ) "mp3_class", ( char* ) "inaktive" );
    setRenderVar ( s, ( char* ) "mp4_class", ( char* ) "inaktive" );
    setRenderVar ( s, ( char* ) "mp5_class", ( char* ) "inaktive" );
    setRenderVar ( s, ( char* ) "mp6_class", ( char* ) "inaktive" );
    setRenderVar ( s, ( char* ) "mp7_class", ( char* ) "inaktive" );
    setRenderVar ( s, ( char* ) "mp8_class", ( char* ) "inaktive" );
    setRenderVar ( s, ( char* ) "mp9_class", ( char* ) "inaktive" );
    setRenderVar ( s, ( char* ) "mp10_class", ( char* ) "inaktive" );
    setRenderVar ( s, ( char* ) "mp11_class", ( char* ) "inaktive" );
    setRenderVar ( s, ( char* ) "mp12_class", ( char* ) "inaktive" );
    setRenderVar ( s, ( char* ) "mp13_class", ( char* ) "inaktive" );
    setRenderVar ( s, ( char* ) "mp14_class", ( char* ) "inaktive" );
    setRenderVar ( s, ( char* ) "mp15_class", ( char* ) "inaktive" );
    setRenderVar ( s, ( char* ) "mp16_class", ( char* ) "inaktive" );

    if ( 0==strcmp ( getRequestURL(s),"index.html" ) ) {
        if ( isRequestSecure(s) == 1 ) {
            setRenderVar ( s, ( char* ) "mp2_class", ( char* ) "aktive" );
        } else {
            setRenderVar ( s, ( char* ) "mp1_class", ( char* ) "aktive" );
        }
    }


    if ( 0==strcmp ( getRequestURL(s),"" ) ) {
        if ( isRequestSecure(s) == 1 ) {
            setRenderVar ( s, ( char* ) "mp2_class", ( char* ) "aktive" );
        } else {
            setRenderVar ( s, ( char* ) "mp1_class", ( char* ) "aktive" );
        }
    }

    if ( 0==strcmp ( getRequestURL(s),"register.html" ) ) {
        if ( isRequestSecure(s) == 1 ) {
            setRenderVar ( s, ( char* ) "mp3_class", ( char* ) "aktive" );
        } else {
            setRenderVar ( s, ( char* ) "mp3_class", ( char* ) "aktive" );
        }
    }

    if ( 0==strcmp ( getRequestURL(s),"unregister.html" ) ) {
        if ( isRequestSecure(s) == 1 ) {
            setRenderVar ( s, ( char* ) "mp4_class", ( char* ) "aktive" );
        } else {
            setRenderVar ( s, ( char* ) "mp4_class", ( char* ) "aktive" );
        }
    }

    if ( 0==strcmp ( getRequestURL(s),"render_variablen.html" ) ) {
        if ( isRequestSecure(s) == 1 ) {
            setRenderVar ( s, ( char* ) "mp6_class", ( char* ) "aktive" );
        } else {
            setRenderVar ( s, ( char* ) "mp5_class", ( char* ) "aktive" );
        }
    }

    if ( 0==strcmp ( getRequestURL(s),"session_storage.html" ) ) {
        if ( isRequestSecure(s) == 1 ) {
            setRenderVar ( s, ( char* ) "mp8_class", ( char* ) "aktive" );
        } else {
            setRenderVar ( s, ( char* ) "mp7_class", ( char* ) "aktive" );
        }
    }

    if ( 0==strncmp ( getRequestURL(s),"cache_test",10 ) ) {
        if ( isRequestSecure(s) == 1 ) {
            setRenderVar ( s, ( char* ) "mp10_class", ( char* ) "aktive" );
        } else {
            setRenderVar ( s, ( char* ) "mp9_class", ( char* ) "aktive" );
        }
    }

    if ( 0==strcmp ( getRequestURL(s),"memory_info.html" ) ) {
        setRenderVar ( s, ( char* ) "mp11_class", ( char* ) "aktive" );
    }

    if ( 0==strcmp ( getRequestURL(s),"websocket.html" ) ) {
        setRenderVar ( s, ( char* ) "mp12_class", ( char* ) "aktive" );
    }

    if ( 0==strcmp ( getRequestURL(s),"ajax_test.html" ) ) {
        if ( isRequestSecure(s) == 1 ) {
            setRenderVar ( s, ( char* ) "mp14_class", ( char* ) "aktive" );
        } else {
            setRenderVar ( s, ( char* ) "mp13_class", ( char* ) "aktive" );
        }
    }
    if ( 0==strcmp ( getRequestURL(s),"if_test.html" ) ) {
        if ( isRequestSecure(s) == 1 ) {
            setRenderVar ( s, ( char* ) "mp16_class", ( char* ) "aktive" );
        } else {
            setRenderVar ( s, ( char* ) "mp15_class", ( char* ) "aktive" );
        }
    }
}

DEFINE_FUNCTION( js_func ){
	printHTML(s,"Hallo C");
}

WEBSERVER_API_HOOK{


    REGISTER_LOCAL_FUNCTION ( checkregister );
	REGISTER_LOCAL_FUNCTION  ( register );
	REGISTER_LOCAL_FUNCTION ( unregister );	
	REGISTER_LOCAL_FUNCTION ( setrvvar );
	REGISTER_LOCAL_FUNCTION ( cachemenustart );
	REGISTER_LOCAL_FUNCTION ( sessionmenustart );
	REGISTER_LOCAL_FUNCTION ( menustart );
    
	REGISTER_LOCAL_FUNCTION ( js_func );

    REGISTER_FUNCTION ( testfunction1 );
    REGISTER_FUNCTION ( testfunction2 );
    REGISTER_FUNCTION ( testfunction3 );
    REGISTER_FUNCTION ( testfunction4 );

    REGISTER_FUNCTION ( if_test_function1 );
    REGISTER_CONDITION ( testcondition1 );
    REGISTER_CONDITION ( testcondition2 );
    REGISTER_CONDITION ( testcondition3 );
    REGISTER_CONDITION ( testcondition4 );

#ifdef WEBSERVER_USE_WEBSOCKETS
    REGISTER_WEBSOCKET_HANDLER ( simple_handler );
    REGISTER_WEBSOCKET_HANDLER ( CommandSocket_handler );
#endif

    startApiThreads();

    return "test_site";
}



