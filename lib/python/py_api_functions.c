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
#include "webserver.h"

#ifdef WEBSERVER_USE_PYTHON

#include "intern/py_plugin.h"
#include "intern/py_utils.h"
#include "intern/py_api_functions.h"
#include "intern/engine.h"

extern http_request *py_cur_s;

#define PY_CONTEXT_CHECK if ( py_cur_s == NULL ){ PyErr_SetString(PyExc_TypeError, "libcwebui.send can only be called from a registered function ");   return NULL;	}


static PyObject* py_send( PyObject* obj, PyObject *args )
{
	PyObject *value;

	PY_CONTEXT_CHECK

	if ( ! PyArg_ParseTuple( args, "O", &value) ){
		PyErr_SetString(PyExc_TypeError, "first parameter must be < string | int >");
		return NULL;
	}

	if ( PyString_Check ( value ) ){
		sendHTMLChunk( py_cur_s->socket, PyString_AsString( value ), strlen( PyString_AsString( value ) ) );
		Py_RETURN_NONE;
	}

	if( PyInt_Check( value ) ){
		char buffer[100];
		int len = snprintf( buffer, 100 , "%d", PyInt_AsLong( value ) );
		sendHTMLChunk( py_cur_s->socket, buffer, len  );
		Py_RETURN_NONE;
	}


	char buffer[100];
	PyObject *type =  PyObject_Type( value );
	snprintf(buffer,100,"Unsuported Type : %s , second parameter must be < string | int >\n" , PyString_AsString( PyObject_Str( type ) ) );

	PyErr_SetString(PyExc_TypeError, buffer);
	return NULL;


}


static PyObject* py_setRenderVar( PyObject* obj, PyObject *args ){

	char *name;
	PyObject *value;

	PY_CONTEXT_CHECK

	if ( ! PyArg_ParseTuple( args, "sO", &name, &value) ){
		PyErr_SetString(PyExc_TypeError, "first parameter must be string, second parameter must be < string | int >");
		return NULL;
	}

	if ( py_obj_is_convertable( value ) == 0 ){
		char buffer[100];
		PyObject *type =  PyObject_Type( value );
		snprintf(buffer,100,"Unsuported Type : %s , second parameter must be < string | int >\n" , PyString_AsString( PyObject_Str( type ) ) );
		PyErr_SetString(PyExc_TypeError, buffer);
		return NULL;
	}

	ws_variable *var =  newVariable(py_cur_s->render_var_store,name);
	py_to_ws_var( var, value );
	Py_RETURN_NONE;
}




static PyObject* py_getRenderVar( PyObject* obj, PyObject *args ){

	char *name;

	PY_CONTEXT_CHECK

	if ( ! PyArg_ParseTuple( args, "s", &name ) ){ PyErr_SetString(PyExc_TypeError, "first parameter must be string"); 	return NULL;}

	ws_variable *var = newVariable(py_cur_s->render_var_store,name);

	PyObject* py_var = py_ws_var_to_py( var );
	if ( py_var == NULL ){
		PyErr_SetString(PyExc_TypeError, "py_getRenderVar: internal error");
		return NULL;
	}

	return py_var;
}


static PyObject* py_setSessionVar( PyObject* obj, PyObject *args ){

	char *store;
	char *name;
	PyObject *value;
	int store_info = -1;

	PY_CONTEXT_CHECK

	if ( ! PyArg_ParseTuple( args, "ssO", &store, &name, &value) ){
		PyErr_SetString(PyExc_TypeError, "first parameter must be string < STORE_NORMAL | STORE_SSL >, second parameter must be string, third parameter must be < string | int >");
		return NULL;
	}

	if ( 0 == strcmp("STORE_NORMAL", store) ){ store_info = SESSION_STORE; }
	if ( 0 == strcmp("STORE_SSL",    store) ){ store_info = SESSION_STORE_SSL; }

	if ( store_info == -1 ){
		PyErr_SetString(PyExc_TypeError, "first parameter must be string < STORE_NORMAL | STORE_SSL >");
		return NULL;
	}

	if ( py_obj_is_convertable( value ) == 0 ){
		char buffer[100];
		PyObject *type =  PyObject_Type( value );
		snprintf(buffer,100,"Unsuported Type : %s , third parameter must be < string | int >\n" , PyString_AsString( PyObject_Str( type ) ) );

		PyErr_SetString(PyExc_TypeError, buffer);
		return NULL;
	}

	ws_variable* var = addSessionValue( py_cur_s , store_info, name);
	py_to_ws_var( var, value );
	Py_RETURN_NONE;
}

static PyObject* py_getSessionVar( PyObject* obj, PyObject *args ){

	char *store;
	char *name;
	int store_info = -1;

	PY_CONTEXT_CHECK

	if ( ! PyArg_ParseTuple( args, "ss", &store,  &name ) ){ PyErr_SetString(PyExc_TypeError, "first parameter must be string"); 	return NULL;}

	if ( 0 == strcmp("STORE_NORMAL", store) ){
		store_info = SESSION_STORE;
	}

	if ( 0 == strcmp("STORE_SSL", store) ){
		store_info = SESSION_STORE_SSL;
	}

	if ( store_info == -1 ){
		PyErr_SetString(PyExc_TypeError, "first parameter must be string < STORE_NORMAL | STORE_SSL >");
		return NULL;
	}

	ws_variable *var = getSessionValue( py_cur_s, store_info, name );

	PyObject* py_var = py_ws_var_to_py( var );
	if ( py_var == NULL ){
		Py_RETURN_NONE;
	}

	return py_var;
}

static PyObject* py_getURLParameter( PyObject* obj, PyObject *args ){

	char *name;
	
	PY_CONTEXT_CHECK

	if ( ! PyArg_ParseTuple( args, "s",  &name ) ){ PyErr_SetString(PyExc_TypeError, "first parameter must be string"); 	return NULL;}

	ws_variable *var = getParameter( py_cur_s, name );

	PyObject* py_var = py_ws_var_to_py( var );
	if ( py_var == NULL ){
		Py_RETURN_NONE;
	}

	return py_var;
}

PyMethodDef py_libcwebui_methods[] =
{
	{"register_function", py_register_function,  METH_VARARGS, "A simple example of an embedded function."},
	{"set_plugin_name", py_set_plugin_name,  METH_VARARGS, "A simple example of an embedded function."},

	{"send", py_send, METH_VARARGS, "A simple example of an embedded function."},

	{"setRenderVar", py_setRenderVar, METH_VARARGS, "A simple example of an embedded function."},
	{"getRenderVar", py_getRenderVar, METH_VARARGS, "A simple example of an embedded function."},

	{"setSessionVar", py_setSessionVar, METH_VARARGS, "A simple example of an embedded function."},
	{"getSessionVar", py_getSessionVar, METH_VARARGS, "A simple example of an embedded function."},

	{"getURLParameter", py_getURLParameter, METH_VARARGS, "A simple example of an embedded function."},
	//setGlobalVar
	//getGlobalVar

	//setVariable

	{ NULL }
};


#endif
