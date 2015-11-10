
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "webserver_api_functions.h"


DEFINE_FUNCTION( func2 ) {
	char buffer[100];

    printHTML( s , "func2 called" );

    setSessionVar ( s,STORE_NORMAL, ( char* ) "session1","value3" );


}


