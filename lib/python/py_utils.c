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

#include "intern/py_utils.h"


void py_print_value ( PyObject* value ){
	//https://docs.python.org/2/c-api/int.html
	//https://docs.python.org/2/c-api/string.html
	//https://docs.python.org/2/c-api/dict.html

	if ( value == 0 ){
		printf("Null Pointer \n");
		return;
	}

	if ( PyString_Check ( value ) ){
		printf("Str  : %s\n", PyString_AsString( value) );
		return;
	}

#if PY_MAJOR_VERSION >= 3
	if( PyInt_Check( value ) ){
		printf("Int  : %ld\n",PyLong_AsLong( value) );
		return;
	}
#else
	if( PyLong_Check( value ) ){
		printf("Int  : %ld\n",PyInt_AsLong( value) );
		return;
	}
#endif

	if( PyDict_Check( value ) ){
		printf("Dict : %zd\n",PyDict_Size( value) );
		PyObject *key, *value_e;
		Py_ssize_t pos = 0;

		while (PyDict_Next( value, &pos, &key, &value_e)) {
			printf("Key : ");py_print_value(key);
			printf("Val : ");py_print_value(value_e);
		}
		return;
	}



	PyObject* type = PyObject_Str( value ) ;
	printf("Unkown Type : %s\n",PyString_AsString( type ) );
}


void py_print_dict_value ( PyObject* dict, char* key ){

	PyObject* value = PyDict_GetItemString( dict, key);

	if( value ){
		py_print_value( value );
	}else{
		printf("%s not found in dict\n",key);
	}
}

const char* py_get_string_param( PyObject *args ){

	PyObject *temp;
	if (! PyArg_ParseTuple(args, "O:set_callback", &temp)){
		PyErr_SetString(PyExc_TypeError, "error parsing parameter");
        return NULL;
	}

	if ( ! PyString_Check ( temp ) ){
		PyErr_SetString(PyExc_TypeError, "argument must ba a string");
        return NULL;
	}

	return PyString_AsString( temp );
}


void py_print_trace( void ){

	PyThreadState *tstate = PyThreadState_GET();

	if (NULL != tstate && NULL != tstate->frame) {
		PyFrameObject *frame = tstate->frame;

		//int line = PyFrame_GetLineNumber( frame);

		const char *filename;
		const char *funcname;

		PyObject* list = PyList_New( 0 );


		const char* last_func = NULL;

		printf("Traceback (most recent call last):\n");
		while (NULL != frame) {

			char buffer[200];

			filename = PyString_AsString(frame->f_code->co_filename);
			funcname = PyString_AsString(frame->f_code->co_name);

			snprintf(buffer, 200 , "  File \"%s\", line %d, in %s\n    %s",
					filename,
					PyFrame_GetLineNumber( frame) ,
					funcname, last_func ? last_func : "register_function");

			last_func = funcname;

			PyList_Append( list, PyString_FromString( buffer ));

			frame = frame->f_back;
		}

		PyList_Reverse( list );

		//printf("Size : %d\n",PyList_Size( list ));
		for ( int i = 0 ; i < PyList_Size( list ); i++ ){
			PyObject* item = PyList_GetItem( list, i );
			puts( PyString_AsString( item ) );
		}

		//printf("  File \"%s\", line %d, in %s\n", filename, line, funcname);

	}
}

PyObject *py_ws_var_to_py( ws_variable *var ){

	if ( var == 0 ) return NULL;

	switch ( var->type ){
		case VAR_TYPE_STRING:
			return PyString_FromString( var->val.value_string );
		case VAR_TYPE_INT:
			return PyInt_FromLong( var->val.value_int);


	}


	return NULL;
}

int py_obj_is_convertable( PyObject *value){

	if( PyString_Check ( value ) ){ return 1; }
	if( PyInt_Check    ( value ) ){ return 1; }

	return 0;
}

void py_to_ws_var( ws_variable* var, PyObject *value){

	if( PyString_Check ( value ) ){ setWSVariableString( var, PyString_AsString( value ) ); }
	if( PyInt_Check    ( value ) ){ setWSVariableInt(    var,    PyInt_AsLong  ( value ) ); }

}

#endif


#if 0

PyObject *iterator = PyObject_GetIter(arg);
	PyObject *item;

	if (iterator != NULL) {

		printf("Iterrating\n");

		while (item = PyIter_Next(iterator)) {
			/* do something with item */
			printf("Item : %s\n", PyString_AsString(  PyObject_Str( item ) ) ) ;
			/* release reference when done */
			Py_DECREF(item);
		}

		Py_DECREF(iterator);

		if (PyErr_Occurred()) {
			/* propagate error */
		}
		else {
			/* continue doing useful work */
		}
	}

#endif
