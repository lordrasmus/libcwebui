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

#ifndef _VARIABLES_GLOBALS_H_
#define _VARIABLES_GLOBALS_H_

//#include "../WebserverConfig.h"

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

