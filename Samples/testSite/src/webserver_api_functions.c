

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "webserver_api_functions.h"


#define OUT_BUFFER_SIZE 10000

void startApiThreads(void);



DEFINE_FUNCTION ( checkregister ) {
    int status;

    status = checkUserRegisterStatus ( s );
    switch ( status ) {
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
        	printHTML( s , "checkUserRegistered unknown return value : %d",status );
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

	setRenderVar ( s, "test1", "test var 1");
	setRenderVar ( s, "test2", "test var 2");
	setRenderVar ( s, "test3", "test var 3");


	var_array = getRenderVar ( s, "testarray1", NO_FLAGS);
	setVariableToArray(var_array);
	addToVarArrayStr(var_array,"entry_2","entry_2_text");
	addToVarArrayStr(var_array,"entry_3","entry_3_text");
	addToVarArrayStr(var_array,"entry_1","entry_1_text");


}

DEFINE_FUNCTION ( cachemenustart ) {
    char buffer[1000];
    char buffer2[1000];
    dummy_var *var;

    setRenderVar ( s, "ctmp1_class", "inaktive" );
    setRenderVar ( s, "ctmp2_class", "inaktive" );
    setRenderVar ( s, "ctmp3_class", "inaktive" );
    setRenderVar ( s, "ctmp4_class", "inaktive" );
    setRenderVar ( s, "ctmp5_class", "inaktive" );
	setRenderVar ( s, "ctmp6_class", "inaktive" );

    setRenderVar ( s, "ctmp11_class", "inaktive" );
    setRenderVar ( s, "ctmp22_class", "inaktive" );
    setRenderVar ( s, "ctmp33_class", "inaktive" );
    setRenderVar ( s, "ctmp44_class", "inaktive" );
    setRenderVar ( s, "ctmp55_class", "inaktive" );
	setRenderVar ( s, "ctmp66_class", "inaktive" );

    var = getRenderVar ( s, "cache_menu", NO_FLAGS);
    if ( var != 0 ) {
		getVariableAsString(var,buffer,1000);
        if ( isRequestSecure(s) == 1 ) {
            snprintf ( buffer2,1000, "ctmp%s%s_class",buffer,buffer );
            setRenderVar ( s,buffer2, "aktive" );
        }
        snprintf ( buffer2,1000, "ctmp%s_class",buffer );
        setRenderVar ( s,buffer2, "aktive" );
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

    FireLoggger (  "menustart from FireLogger" );



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
    setRenderVar ( s, ( char* ) "mp17_class", ( char* ) "inaktive" );

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
    if ( 0==strcmp ( getRequestURL(s),"python.html" ) ) {
       setRenderVar ( s, ( char* ) "mp17_class", ( char* ) "aktive" );
    }
}

DEFINE_FUNCTION( js_func ){
	printHTML(s,"Hallo C");
}

DEFINE_FUNCTION( file_upload ){
	printf("Files : %d\n",getFileCount(s));
	printHTML(s,"Files : %d<br>",getFileCount(s));
	if ( getFileCount(s) > 0 ){
		printf("Name  : %s\n",getFileName(s,0));
		printf("Size  : %d\n",getFileSize(s,0));
		
		printHTML(s,"Name  : %s<br>",getFileName(s,0));
		printHTML(s,"Size  : %d<br>",getFileSize(s,0));
		
		char *data = getFileData(s,0);
		FILE* fd = fopen("out.dat","w");
		fwrite( data, getFileSize(s,0), 1 , fd );
		fclose(fd);
		
		system("sha256sum out.dat");
	}
}



#ifdef SINGLE_MAIN
void init_testsite( void ){
#else
WEBSERVER_API_HOOK{	
#endif

	REGISTER_LOCAL_FUNCTION( file_upload );

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

#ifndef SINGLE_MAIN
    return "test_site";
#endif
}





