#include "stdafx.h"

#include <iostream>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <syslog.h>
#include <dlfcn.h>
#include <sys/socket.h>
#include <netdb.h>


#include "convert.h"
#include "files.h"
#include "server.h"
#include "system.h"
#include "utils.h"
#include "engine.h"

#include "variable_render.h"
#include "variable_store.h"
#include "variables_globals.h"
#include "webserver_api_functions.h"
#include "globals.h"
#include "session.h"


#include "system_sockets.h"


extern unsigned long allocated;

void var_tests() {
    char buffer[100];
    ws_variable_store v_store;
    ws_variable *var;
    int tmp_i;

    printf("Teste Variablen Store\n");

    printf("Memory : %ld\n",allocated);

    initVariableStore(&v_store);

    var = newVariable(&v_store,"test1");
    setWSVariableString(var,"var test1");
    printf("Memory : %ld\n",allocated);

    var = newVariable(&v_store,"test2");
    setWSVariableString(var,"var test_2");
    printf("Memory : %ld\n",allocated);

    var = newVariable(&v_store,"test3");
    setWSVariableInt(var,1);
    printf("Memory : %ld\n",allocated);

    var = newVariable(&v_store,"test4");
    setWSVariableInt(var,2);
    printf("Memory : %ld\n",allocated);



    var = getVariable(&v_store,"test1");
    getWSVariableString(var,buffer,100);
    printf("%s\n",buffer);

    var = getVariable(&v_store,"test2");
    getWSVariableString(var,buffer,100);
    printf("%s\n",buffer);

    var = getVariable(&v_store,"test3");
    getWSVariableString(var,buffer,100);
    printf("%s\n",buffer);

    var = getVariable(&v_store,"test4");
    getWSVariableString(var,buffer,100);
    printf("%s\n",buffer);

    var = getVariable(&v_store,"test1");
    tmp_i = getWSVariableInt(var);
    printf("%d\n",tmp_i);

    var = getVariable(&v_store,"test2");
    tmp_i = getWSVariableInt(var);
    printf("%d\n",tmp_i);

    var = getVariable(&v_store,"test3");
    tmp_i = getWSVariableInt(var);
    printf("%d\n",tmp_i);

    var = getVariable(&v_store,"test4");
    tmp_i = getWSVariableInt(var);
    printf("%d\n",tmp_i);

    var = getVariable(&v_store,"test4");
    freeVariable(&v_store,var);
    printf("Memory : %ld\n",allocated);

    var = getVariable(&v_store,"test3");
    freeVariable(&v_store,var);
    printf("Memory : %ld\n",allocated);

    var = getVariable(&v_store,"test2");
    freeVariable(&v_store,var);
    printf("Memory : %ld\n",allocated);

    var = getVariable(&v_store,"test1");
    freeVariable(&v_store,var);
    printf("Memory : %ld\n",allocated);


    var = newVariable(&v_store,"test1");
    setWSVariableString(var,"var test1");
    printf("Memory : %ld\n",allocated);
    
    var = newVariable(&v_store,"test1");
    setWSVariableString(var,"var test1");
    printf("Memory : %ld\n",allocated);

    var = newVariable(&v_store,"test2");
    setWSVariableString(var,"var test2");
    printf("Memory : %ld\n",allocated);


    clearVariables(&v_store);
    printf("Memory : %ld\n",allocated);
    
       
    
    printf("Memory : %ld\n",allocated);

    
    
}


void global_var_test(){
	char buffer[100];
	ws_variable *var;

	printf("Teste Globale Variablen\n");

	setGlobalVariable("g_test1","test11");
	printf("Memory : %ld\n",allocated);

	setGlobalVariable("g_test1","test1");
	printf("Memory : %ld\n",allocated);

	setGlobalVariable("g_test2","test2");
	printf("Memory : %ld\n",allocated);

	var = getGlobalVariable("g_test1");
	getWSVariableString(var,buffer,100);
	printf("%s\n",buffer);

	var = getGlobalVariable("g_test2");
	getWSVariableString(var,buffer,100);
	printf("%s\n",buffer);
}


void session_var_test(){
	session s;
	ws_variable *var;

	printf("Teste Session Variablen\n");

	memset(&s,0,sizeof(session));
	s.socket = WebserverMallocSocketInfo();
	s.socket->use_ssl = 1;

	createSession(&s,0);
	createSession(&s,1);


	setSessionValue(&s,SESSION_STORE,"stest1","test1");
	setSessionValue(&s,SESSION_STORE,"stest2","test2");
	setSessionValue(&s,SESSION_STORE_SSL,"stest3","test3");
	setSessionValue(&s,SESSION_STORE_SSL,"stest4","test4");

	var = getSessionValue(&s,SESSION_STORE,"stest1");
	var = getSessionValue(&s,SESSION_STORE,"stest2");
	var = getSessionValue(&s,SESSION_STORE,"stest3");
	var = getSessionValue(&s,SESSION_STORE,"stest4");

	var = getSessionValue(&s,SESSION_STORE_SSL,"stest1");
	var = getSessionValue(&s,SESSION_STORE_SSL,"stest2");
	var = getSessionValue(&s,SESSION_STORE_SSL,"stest3");
	var = getSessionValue(&s,SESSION_STORE_SSL,"stest4");

	printf("Teste Session Variablen beendet\n");

	return;
}

void render_var_test(){
	session s;
	ws_variable *var;

	printf("Teste Render Variablen\n");

	memset(&s,0,sizeof(session));

}

