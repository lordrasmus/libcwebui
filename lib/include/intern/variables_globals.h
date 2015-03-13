/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/


#ifndef _VARIABLES_GLOBALS_H_
#define _VARIABLES_GLOBALS_H_


#include "dataTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

	void 			initGlobalVariable(void);
	void 			freeGlobalVariable(void);
	ws_variable* 	getGlobalVariable(const char* name);
	ws_variable* 	getExistentGlobalVariable(const char* name);
	ws_variable* 	setGlobalVariable(const char* name,const char* text);
	void			dumpGlobals(http_request* s);

	int	NEED_RESUL_CHECK lockGlobals(void);
	int	NEED_RESUL_CHECK unlockGlobals(void);

#ifdef __cplusplus
}
#endif

#endif /* _VARIABLES_GLOBALS_H_ */

