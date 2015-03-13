/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/

#include "webserver.h"

void WebserverGenerateGUID(char* buf, int length){ 	
	generateGUID(buf,length); 
}

void setRenderVar(dummy_handler* s, char* name, char* text) { 
	ws_set_render_var( s, name, text ); 
}

dummy_var* getRenderVar(dummy_handler* s, const char* name,WS_VAR_FLAGS flags) { 
	return ws_get_render_var( s, name, flags ); 
}

void WebserverAddFileDir(const char* alias,const char* dir){
	add_local_file_system_dir(alias,dir,1,0);
}

void WebserverAddFileDirNoCache(const char* alias,const char* dir){
	add_local_file_system_dir(alias,dir,0,0);
}

