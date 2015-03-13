
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "webserver_api_functions.h"


#ifdef _MSC_VER
#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS
#endif




DEFINE_FUNCTION( testfunction1 ) {
	char buffer[100];
	dummy_var *var,*var2;

    FireLoggger ( "testfunction1" );
    var = getURLParameter ( s, (char*) "testpara1" );
    if ( var != 0 ) {
		getVariableAsString(var,buffer,100);
		FireLoggger (  "Para testpara1 : %s", buffer );

        setSessionVar ( s,STORE_NORMAL, ( char* ) "test1",buffer );
        var2 = getURLParameter ( s, ( char* ) "testpara2" );
		if ( var2!=0 ){
			getVariableAsString(var2,buffer,100);
            setSessionVar ( s,STORE_NORMAL, ( char* ) "test2",buffer );
		}
		sendHTMLVariable(s,var);
    } else {
        printHTML( s , "testfunction1 hat nichts zu tun" );
    }
}

DEFINE_FUNCTION( testfunction2 ) {
	dummy_var *var;
	char buffer[100];
    FireLoggger ( "testfunction2" );
    var = getURLParameter ( s, ( char* ) "testpara2" );
    if ( var != 0 ) {
		getVariableAsString(var,buffer,100);
        if ( 0==strcmp ( buffer, ( char* ) "1" ) )
        	setSessionVar ( s,STORE_NORMAL, ( char* ) "test3", ( char* ) "nen Text" );
        else if ( 0==strcmp ( buffer, ( char* ) "2" ) )
        	setSessionVar ( s,STORE_NORMAL, ( char* ) "test3", ( char* ) "nen Anderer" );
        else
        	setSessionVar ( s,STORE_NORMAL, ( char* ) "test3", ( char* ) "irgendwas" );
    } else {
    	setSessionVar ( s,STORE_NORMAL, ( char* ) "test3", ( char* ) "kein parameter testpara2" );
    }
}

DEFINE_FUNCTION( testfunction3 ) {
    char buffer[1000];
    dummy_var *var;
    var = getSessionVar ( s,STORE_NORMAL, ( char* ) "test2", NO_FLAGS);
    if (var != 0) {
    	getVariableAsString(var,buffer,100);
		if ( 0==strcmp ( buffer,"1" ) ){
            printHTML( s , "<table><tr><td>1<tr><td>2<tr><td>3<tr><td>4</table>" );
		}else if ( 0==strcmp ( buffer,"2" ) ){
                printHTML( s , "<table><tr><td>4<tr><td>3<tr><td>2<tr><td>1</table>" );
		}else if ( 0==strcmp ( buffer,"3" ) ){
                    printHTML( s , "<table><tr><td>2<tr><td>2<tr><td>2<tr><td>2</table>" );
		}else{
                        printHTML( s , "<table><tr><td>1<tr><td>1<tr><td>1<tr><td>1</table>" );
		}
	}else{
		printHTML( s , "<table><tr><td>2<tr><td>2<tr><td>2<tr><td>2</table>" );
	}
}

DEFINE_FUNCTION( testfunction4 ) {	
    char buffer[1000];
    dummy_var *var;

    var = getURLParameter ( s, "testpara2" );
    if ( var !=0 ) {
		getVariableAsString(var,buffer,100);
        setSessionVar ( s,STORE_NORMAL, "test2",buffer );
		setSessionVar ( s,STORE_SSL, "test2",buffer );
    }

    var = getSessionVar ( s,STORE_NORMAL, "test2", NO_FLAGS);
    if ( var != 0 ) {
    	getVariableAsString(var,buffer,1000);
        FireLoggger ( "ssl test2 : %s",buffer );
    } else {
        FireLoggger ( "ssl test2 not found" );
    }

    var = getSessionVar ( s,STORE_NORMAL, "test2", NO_FLAGS);
    if ( var != 0 ) {
    	getVariableAsString(var,buffer,1000);
        FireLoggger ( "test2 : %s",buffer );
    } else {
        FireLoggger ( "test2 not found" );
    }
}

DEFINE_FUNCTION( if_test_function1 ){
	char buffer[100];
	dummy_var *var1 = getURLParameter ( s, "testcond1toogle" );
	dummy_var *var2 = getURLParameter ( s, "testcond2toogle" );
	dummy_var *var3 = getURLParameter ( s, "testcond3toogle" );
	dummy_var *var4 = getURLParameter ( s, "testcond4toogle" );

	dummy_var *var;

	if(var1 != 0){
		var = getSessionVar ( s,STORE_NORMAL, ( char* ) "testcond1status", NO_FLAGS);
		if ( var != 0 ) {
			 getVariableAsString(var,buffer,100);
			 if(0==strcmp(buffer,"true")){
				setSessionVar ( s,STORE_NORMAL, ( char* ) "testcond1status","false" );
			 }else{
				setSessionVar ( s,STORE_NORMAL, ( char* ) "testcond1status","true" );
			 }
		}else{
			setSessionVar ( s,STORE_NORMAL, ( char* ) "testcond1status","true" );
		}		
	}

	if(var2 != 0){
		var = getSessionVar ( s,STORE_NORMAL, ( char* ) "testcond2status", NO_FLAGS);
		if ( var != 0 ) {
			 getVariableAsString(var,buffer,100);
			 if(0==strcmp(buffer,"true")){
				setSessionVar ( s,STORE_NORMAL, ( char* ) "testcond2status","false" );
			 }else{
				setSessionVar ( s,STORE_NORMAL, ( char* ) "testcond2status","true" );
			 }
		 }else{
			setSessionVar ( s,STORE_NORMAL, ( char* ) "testcond2status","true" );
		 }	
	}

	if(var3 != 0){
		var = getSessionVar ( s,STORE_NORMAL, ( char* ) "testcond3status", NO_FLAGS);
		if ( var != 0 ) {
			 getVariableAsString(var,buffer,100);
			 if(0==strcmp(buffer,"true")){
				setSessionVar ( s,STORE_NORMAL, ( char* ) "testcond3status","false" );
			 }else{
				setSessionVar ( s,STORE_NORMAL, ( char* ) "testcond3status","true" );
			 }
		 }else{
			setSessionVar ( s,STORE_NORMAL, ( char* ) "testcond3status","true" );
		 }	
	}

	if(var4 != 0){
		var = getSessionVar ( s,STORE_NORMAL, ( char* ) "testcond4status", NO_FLAGS);
		if ( var != 0 ) {
			 getVariableAsString(var,buffer,100);
			 if(0==strcmp(buffer,"true")){
				setSessionVar ( s,STORE_NORMAL, ( char* ) "testcond4status","false" );
			 }else{
				setSessionVar ( s,STORE_NORMAL, ( char* ) "testcond4status","true" );
			 }
		 }else{
			setSessionVar ( s,STORE_NORMAL, ( char* ) "testcond4status","true" );
		 }	
	}
}

DEFINE_CONDITION( testcondition1 ) {
	char buffer[100];
	dummy_var *var = getSessionVar ( s,STORE_NORMAL, ( char* ) "testcond1status", NO_FLAGS);
	if ( var != 0 ) {
		getVariableAsString(var,buffer,100);
		if(0==strcmp(buffer,"true")){
			return COND_TRUE;
		}
	}
	return COND_FALSE;
}

DEFINE_CONDITION( testcondition2 ) {
	char buffer[100];
	dummy_var *var = getSessionVar ( s,STORE_NORMAL, ( char* ) "testcond2status", NO_FLAGS);
	if ( var != 0 ) {
		getVariableAsString(var,buffer,100);
		if(0==strcmp(buffer,"true")){
			return COND_TRUE;
		}
	}
	return COND_FALSE;
}

DEFINE_CONDITION( testcondition3 ) {
	char buffer[100];
	dummy_var *var = getSessionVar ( s,STORE_NORMAL, ( char* ) "testcond3status", NO_FLAGS);
	if ( var != 0 ) {
		getVariableAsString(var,buffer,100);
		if(0==strcmp(buffer,"true")){
			return COND_TRUE;
		}
	}
	return COND_FALSE;
}

DEFINE_CONDITION( testcondition4 ) {
	char buffer[100];
	dummy_var *var = getSessionVar ( s,STORE_NORMAL, ( char* ) "testcond4status", NO_FLAGS);
	if ( var != 0 ) {
		getVariableAsString(var,buffer,100);
		if(0==strcmp(buffer,"true")){
			return COND_TRUE;
		}
	}
	return COND_FALSE;
}
