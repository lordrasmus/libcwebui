/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/


#ifndef _WEBSERVER_PY_PLUGINS_H_

#ifdef WEBSERVER_USE_PYTHON

#include "webserver.h"
#include "red_black_tree.h"

#include "intern/py_utils.h"


#if PY_MAJOR_VERSION >= 3
	#define PyInt_Check    PyLong_Check
	#define PyInt_FromLong PyLong_FromLong
	#define PyInt_AsLong   PyLong_AsLong


	#define PyString_Check        PyUnicode_Check
	#define PyString_AsString     PyUnicode_AsUTF8
	#define PyString_FromString   PyUnicode_FromString
#endif

int py_init_modules( void );

int py_load_python_plugin( const char* path );

void py_call_engine_function( http_request *s, user_func_s *func , FUNCTION_PARAS* paras );

PyObject* py_register_function( PyObject* self, PyObject *args );
PyObject* py_set_plugin_name( PyObject* self, PyObject *args );


#endif

#endif



