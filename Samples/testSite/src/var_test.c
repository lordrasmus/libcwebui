#include "stdafx.h"

#ifdef khlskdjfhldskjh

#include "webserver.h"

extern unsigned long allocated;
extern unsigned long allocated_real;

ws_variable* insertStoreValStr(ws_variable_store *v_store, const char* name,
		const char* value) {
	int size;
	unsigned long size_store;
	ws_variable *var = newVariable(v_store, name);
	setWSVariableString(var, value);
	size = getWSVariableSize(var);
	size_store = getVariableStoreSize(v_store);
	printf("Memory Allocated :%ld\tVar :%d\tStore :%ld\n", allocated, size,
			size_store);
	return var;
}

ws_variable* insertStoreValInt(ws_variable_store *v_store, const char* name,
		int value) {
	int size;
	unsigned long size_store;
	ws_variable *var = newVariable(v_store, name);
	setWSVariableInt(var, value);
	size = getWSVariableSize(var);
	size_store = getVariableStoreSize(v_store);
	printf("Memory Allocated :%ld\tVar :%d\tStore :%ld\n", allocated, size,
			size_store);
	return var;
}

void printVar(ws_variable_store* v_store, const char* name) {
	char buffer[100];
	int tmp_i;
	ws_variable *var;
	var = getVariable(v_store, name);
	tmp_i = getWSVariableInt(var);
	printf("Int :%d", tmp_i);
	getWSVariableString(var, buffer, 100);
	printf("\tString :%s\n", buffer);

}

void freeVar(ws_variable_store* v_store, const char* name) {
	unsigned long size_store;
	ws_variable* var = getVariable(v_store, name);
	freeVariable(v_store, var);
	size_store = getVariableStoreSize(v_store);
	printf("Memory Allocated :%ld\tStore :%ld\n", allocated, size_store);
}

void printArrayIndex(ws_variable *var_array) {
	unsigned int i;
	char buffer[100];
	ws_variable *var;

	for (i = 0; i < 10; i++) {
		var = getWSVariableArrayIndex(var_array, i);
		if (var == 0)
			break;
		getWSVariableString(var, buffer, 100);
		printf("%s \t-> %s\n", var->name, buffer);
	}
}

void var_tests() {
	char buffer[100];
	ws_variable_store *v_store = WebserverMalloc(sizeof(ws_variable_store),0);
	ws_variable *var, *var_array;
//    int tmp_i;//,size;
	unsigned long size_store;
//	int i;

	printf("Teste Variablen Store\n");

	printf("Memory : %ld\n", allocated);
	initVariableStore(v_store);
	printf("Memory : %ld\n", allocated);

	var = insertStoreValStr(v_store, "test1", "var test1");
	var = insertStoreValStr(v_store, "test1", "var test1");
	var = insertStoreValStr(v_store, "test2", "var test_2");
	var = insertStoreValInt(v_store, "test3", 1);
	var = insertStoreValInt(v_store, "test4", 2);

	printVar(v_store, "test1");
	printVar(v_store, "test2");
	printVar(v_store, "test3");
	printVar(v_store, "test4");

	//setVariableStoreFirst(v_store);

	var = getFirstVariable(v_store);
	while (var != 0) {
		getWSVariableString(var, buffer, 100);
		printf("%s\n", buffer);
		var = getNextVariable(v_store);
	}
	//getWSVariableString(var,buffer,100);
	//printf("%s\n",buffer);

	freeVar(v_store, "test4");
	freeVar(v_store, "test4");
	freeVar(v_store, "test3");
	freeVar(v_store, "test2");
	freeVar(v_store, "test1");

	var = insertStoreValStr(v_store, "test1", "var test1");
	var = insertStoreValStr(v_store, "test1", "var test1");
	var = insertStoreValStr(v_store, "test2", "var test2");

	clearVariables(v_store);
	printf("Memory : %ld\n", allocated);

	var_array = newVariable(v_store, "array_test_1");
	setWSVariableArray(var_array);
	var = getWSVariableArray(var_array, "entry_1");

	var = addWSVariableArray(var_array, "entry_1");
	getWSVariableString(var, buffer, 100);
	printf("%s\n", buffer);
	size_store = getVariableStoreSize(v_store);
	printf("Memory Allocated :%ld\tStore :%ld\n", allocated, size_store);

	setWSVariableString(var, "entry_1_text");
	getWSVariableString(var, buffer, 100);
	printf("%s\n", buffer);
	size_store = getVariableStoreSize(v_store);
	printf("Memory Allocated :%ld\tStore :%ld\n", allocated, size_store);

	var = getWSVariableArray(var_array, "entry_1");
	getWSVariableString(var, buffer, 100);
	printf("%s\n", buffer);
	size_store = getVariableStoreSize(v_store);
	printf("Memory Allocated :%ld\tStore :%ld\n", allocated, size_store);

	var = addWSVariableArray(var_array, "entry_2");
	setWSVariableString(var, "entry_2_text");
	getWSVariableString(var, buffer, 100);
	printf("%s\n", buffer);
	size_store = getVariableStoreSize(v_store);
	printf("Memory Allocated :%ld\tStore :%ld\n", allocated, size_store);

	var = getWSVariableArrayFirst(var_array);
	while (var != 0) {
		getWSVariableString(var, buffer, 100);
		printf("%s\n", buffer);
		var = getWSVariableArrayNext(var_array);
	}

	delWSVariableArray(var_array, "entry_1");
	size_store = getVariableStoreSize(v_store);
	printf("Memory Allocated :%ld\tStore :%ld\n", allocated, size_store);

	var = getWSVariableArrayFirst(var_array);
	while (var != 0) {
		getWSVariableString(var, buffer, 100);
		printf("%s\n", buffer);
		var = getWSVariableArrayNext(var_array);
	}

	printArrayIndex(var_array);

	var = addWSVariableArray(var_array, "entry_1");
	setWSVariableString(var, "entry_1_text");
	var = addWSVariableArrayIndex(var_array, 0);
	var = addWSVariableArrayIndex(var_array, 1);
	var = addWSVariableArrayIndex(var_array, 2);
	var = addWSVariableArrayIndex(var_array, 3);
	var = addWSVariableArrayIndex(var_array, 6);

	size_store = getVariableStoreSize(v_store);
	printf("Memory Allocated :%ld\tStore :%ld\n", allocated, size_store);

	printArrayIndex(var_array);

	//clearVariables(v_store);
	deleteVariableStore(v_store);

	printf("Memory : %ld\n", allocated);

}

void test_var_store_list() {
	ws_variable *var1, *var2, *var3, *var_tmp;

	printf("Memory Allocated :%ld\n", allocated);

	ws_variable_store *store = createVariableStore();

	printf("Memory Allocated :%ld\n", allocated);

	var1 = newVariable(store, "Var1");
	printf("The list now holds %u elements.\n", getVariableStoreSize(store));
	var2 = newVariable(store, "Var2");
	printf("The list now holds %u elements.\n", getVariableStoreSize(store));
	printf("Memory Allocated :%ld\n", allocated);
	var3 = newVariable(store, "Var3");
	printf("The list now holds %u elements.\n", getVariableStoreSize(store));
	printf("Memory Allocated :%ld\n", allocated);

	var_tmp = getVariable(store, "Var1");
	var_tmp = getVariable(store, "Var3");

	lockStore(store);
	var_tmp = getFirstVariable(store);
	var_tmp = getNextVariable(store);
	var_tmp = getNextVariable(store);
	var_tmp = getNextVariable(store);
	unlockStore(store);

	freeVariable(store, var2);
	printf("The list now holds %u elements.\n", getVariableStoreSize(store));
	printf("Memory Allocated :%ld\n", allocated);

	clearVariables(store);
	printf("The list now holds %u elements.\n", getVariableStoreSize(store));
	printf("Memory Allocated :%ld\n", allocated);

	clearVariables(store);

	deleteVariableStore(store);
	printf("Memory Allocated :%ld\n", allocated);

}

void list_test() {

	//var1 = newWSVariable("Var1");
	//var2 = newWSVariable("Var2");

	/*list_t mylist;
	 list_init(&mylist);
	 list_attributes_seeker(& mylist, var_seeker);

	 list_append(& mylist, var1);
	 printf("The list now holds %u elements.\n",  list_size(& mylist));

	 list_append(& mylist, var2);
	 printf("The list now holds %u elements.\n",  list_size(& mylist));


	 printf("The list now holds %u elements.\n",  list_size(& mylist));

	 var_tmp = (ws_variable *)list_seek(& mylist, "Var1");
	 var_tmp = (ws_variable *)list_seek(& mylist, "Var3");

	 list_destroy(&mylist);*/
}

void global_var_test() {
	char buffer[100];
	ws_variable *var;

	printf("Teste Globale Variablen\n");

	setGlobalVariable("g_test1", "test11");
	printf("Memory : %ld\n", allocated);

	setGlobalVariable("g_test1", "test1");
	printf("Memory : %ld\n", allocated);

	setGlobalVariable("g_test2", "test2");
	printf("Memory : %ld\n", allocated);

	var = getGlobalVariable("g_test1");
	getWSVariableString(var, buffer, 100);
	printf("%s\n", buffer);

	var = getGlobalVariable("g_test2");
	getWSVariableString(var, buffer, 100);
	printf("%s\n", buffer);
}

void session_var_test() {
	http_request s;
	ws_variable *var;

	printf("Teste Session Variablen\n");

	memset(&s, 0, sizeof(http_request));
	s.socket = WebserverMallocSocketInfo();
	s.socket->use_ssl = 1;

	createSession(&s, 0);
	createSession(&s, 1);

	setSessionValue(&s, SESSION_STORE, "stest1", "test1");
	setSessionValue(&s, SESSION_STORE, "stest2", "test2");
	setSessionValue(&s, SESSION_STORE_SSL, "stest3", "test3");
	setSessionValue(&s, SESSION_STORE_SSL, "stest4", "test4");

	var = getSessionValue(&s, SESSION_STORE, "stest1");
	var = getSessionValue(&s, SESSION_STORE, "stest2");
	var = getSessionValue(&s, SESSION_STORE, "stest3");
	var = getSessionValue(&s, SESSION_STORE, "stest4");

	var = getSessionValue(&s, SESSION_STORE_SSL, "stest1");
	var = getSessionValue(&s, SESSION_STORE_SSL, "stest2");
	var = getSessionValue(&s, SESSION_STORE_SSL, "stest3");
	var = getSessionValue(&s, SESSION_STORE_SSL, "stest4");

	printf("Teste Session Variablen beendet\n");

	return;
}

void render_var_test() {
	http_request s;
//	ws_variable *var;

	printf("Teste Render Variablen\n");

	memset(&s, 0, sizeof(http_request));

}

#endif

