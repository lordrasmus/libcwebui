/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/

#ifndef _WEBSERVER_PY_UTILS_H_
#define _WEBSERVER_PY_UTILS_H_


#include <Python.h>
#include <frameobject.h>
#include "intern/engine.h"


void py_print_value ( PyObject* value );
void py_print_dict_value ( PyObject* dict, char* key );
void py_print_trace( void );

const char* py_get_string_param( PyObject *args );

int       py_obj_is_convertable( PyObject *value);

PyObject *py_ws_var_to_py( ws_variable *var );
void      py_to_ws_var( ws_variable* var, PyObject *value);

#endif
