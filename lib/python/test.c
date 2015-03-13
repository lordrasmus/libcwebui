/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/

#include "py_utils.h"



/*
 http://python3porting.com/cextensions.html
 https://docs.python.org/2/extending/extending.html
 https://mathieu.fenniak.net/embedding-python-tips/
 https://docs.python.org/2/c-api/capsule.html#capsules
 https://docs.python.org/2/reference/datamodel.html
 https://docs.python.org/2/c-api/
 http://www.tutorialspoint.com/python/python_further_extensions.htm
*/

PyObject* send( PyObject* obj, PyObject *args )
{
	char* param = get_string_param( args );

	puts(param);

	Py_RETURN_NONE;
}

struct web_py_module{
	PyThreadState* thread_state;
	PyObject* global_namespace;
	PyObject* local_namespace;
	char* path;
	char plugin_name[60];
};



static struct web_py_module *context= NULL;


static char* plugin_name = NULL;
static char* plugin_name_buff = NULL;

static PyObject *func_dict = NULL;


PyObject* register_function( PyObject* self, PyObject *args )
{
	PyObject *result = NULL;
	PyObject *func_obj = NULL;
    PyObject *temp;
    char* func_name;

    if ( plugin_name == NULL ){
		PyErr_SetString(PyExc_TypeError, "plugin name must be set first with set_plugin_name");
        return NULL;
	}

    if (! PyArg_ParseTuple(args, "O:set_callback", &temp)){
		PyErr_SetString(PyExc_TypeError, "error parsing parameter");
        return NULL;
	}

	if (!PyCallable_Check(temp)) {
		PyErr_SetString(PyExc_TypeError, "parameter must be callable");
		return NULL;
	}

	Py_XINCREF(temp);         /* Add a reference to new callback */

	//my_callback = temp;       /* Remember new callback */

    func_obj =  PyObject_GenericGetAttr( temp, PyString_FromString( "__name__" ) );
	func_name = PyString_AsString( func_obj );

	printf("register python function <%s> \n",func_name);

	if ( PyDict_Contains( func_dict, func_obj ) ){
		PyErr_SetString(PyExc_TypeError, "function name already registered");
		return NULL;
	}

	PyDict_SetItem( func_dict, func_obj, temp );

	if ( context == 0 ){

		print_py_trace();

		PyErr_SetString(PyExc_TypeError, "libcwebuiError: no Context, skipping python function register\n");
		Py_RETURN_NONE;
	}

	Py_RETURN_NONE;
}




PyObject* set_plugin_name( PyObject* self, PyObject *args )
{
	if ( context == NULL ){
		PyErr_SetString(PyExc_TypeError, "internal server error:  set_plugin_name : context == NULL\n");
		return NULL;
	}

	if ( plugin_name_buff == NULL ){
		PyErr_SetString(PyExc_TypeError, "internal server error:  set_plugin_name : plugin_name_buff == NULL\n");
		return NULL;
	}

	char* param = get_string_param( args );

	plugin_name = plugin_name_buff;
	strncpy(plugin_name_buff, param , 59);

	printf("python plugin name set to <%s>\n",plugin_name);

	Py_RETURN_NONE;
}

void load_python_plugin( char* path ){
	struct web_py_module * mod = malloc( sizeof( struct web_py_module ) );

	mod->thread_state = Py_NewInterpreter();
	mod->global_namespace = PyModule_GetDict( PyImport_AddModule("__main__") );
	//mod->local_namespace  = PyDict_New();

	mod->path = malloc ( strlen ( path ) + 1 ) ;
	strcpy( mod->path , path );

	plugin_name = NULL;
	plugin_name_buff = mod->plugin_name;

	printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
	printf("loading python plugin : %s  start \n",path);
	printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");

	FILE* fp = fopen(path,"r");
	if ( fp ){

		context = mod;
		//context = NULL;

		PyRun_File( fp,path , Py_file_input, mod->global_namespace,  mod->global_namespace );

		context = NULL;

		fclose(fp);
	}

	PyErr_Print();


	printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
	printf("loading python plugin : %s finished\n",path);
	printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");

	//return;

	//printf("----------  Function --------------\n");
	//print_value( my_callback );
	//printf("----------  Globals ---------------\n");
	//print_value( mod->global_namespace );
	/*printf("----------  Locals  ---------------\n");
	print_value( mod->local_namespace );
	printf("-----------------------------------\n");

	PyDict_Merge( mod->global_namespace, mod->local_namespace , 1);
	*/
}

void call_py_function( char* name ){

	int arg = 22;
	PyObject *arglist;
	PyObject *result;

	PyObject *my_callback = PyDict_GetItem( func_dict, PyString_FromString( name ) );

	//PyGILState_STATE gstate;
	//gstate = PyGILState_Ensure();

	arglist = Py_BuildValue("(i)", arg);
	result = PyObject_CallObject(my_callback, arglist);
	Py_XDECREF(arglist);
	Py_XDECREF(result);
	PyErr_Print();


	// so müsste man an die locals kommen
	// PyObject_Dir( my_callback )


	//PyGILState_Release(gstate);

}

PyMethodDef libcwebui_methods[] =
{
	{"register_function", register_function,  METH_VARARGS, "A simple example of an embedded function."},
	{"set_plugin_name", set_plugin_name,  METH_VARARGS, "A simple example of an embedded function."},


	{"send", send, METH_VARARGS, "A simple example of an embedded function."},

	NULL
};

#if PY_MAJOR_VERSION >= 3
struct PyModuleDef libcwebui_module = {
	PyModuleDef_HEAD_INIT,
	"hello",
	NULL,
	-1,
	libcwebui_methods,
	NULL, NULL, NULL, NULL
};
#endif

PyMODINIT_FUNC py_init(void){
	//printf("libcwebui Python Module Init\n");

	#if PY_MAJOR_VERSION >= 3
		PyModule_Create(&libcwebui_module);
	#else
		Py_InitModule3("libcwebui", libcwebui_methods, "This is a module");
	#endif

    func_dict = PyDict_New();
}

int main(){

	Py_SetProgramName("libcwebui");

	if (PyImport_AppendInittab( "libcwebui", py_init) == -1){
		printf("Failed to add libcwebui to the interpreter's builtin modules");
		PyErr_Print();
		return 1;
    }

	Py_Initialize();
	//PyEval_InitThreads();

	load_python_plugin("test.py");
	load_python_plugin("test2.py");


	call_py_function( "test" );
	call_py_function( "test2" );
	call_py_function( "test3" );
	call_py_function( "test" );



	return 0;

	/*result = PyObject_GenericGetAttr( my_callback, PyString_FromString( "__globals__" ) );
	print_value( result );


	result = PyObject_GenericGetAttr( my_callback, PyString_FromString( "__all__" ) );
	print_value( result );

	result = PyObject_GetAttr( my_callback, PyString_FromString( "g" ) );
	print_value( result );*/

	//print_dict_value(  global_namespace, "b");

	return 0;


	//PyGILState_Release(gstate);




	//Py_Finalize();

	return 0;
}







//PyRun_String( "import pprint" , Py_file_input, global_namespace, local_namespace );
	//PyErr_Print();
	//PyRun_String( "print(\"a\")" , Py_file_input, global_namespace, local_namespace );
	//PyErr_Print();

	//PyCodeObject* code = Py_CompileString( "print(\"a\")" ,"<internal>",0);
	//PyEval_EvalCode( code , global_namespace, local_namespace );


