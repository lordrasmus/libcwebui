/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/


#ifndef RENDER_VARIABLE_H_
#define RENDER_VARIABLE_H_



#include "webserver.h"

#ifdef __cplusplus
extern "C" {
#endif

void 			initRenderVariable(http_request* s);
ws_variable*  	getRenderVariable(http_request* s,const char* name);
void			setRenderVariable(http_request* s,const char* name,const char* text);
void 			clearRenderVariables(http_request* s);


#ifdef __cplusplus
}
#endif

#endif /* RENDER_VARIABLE_H_ */
