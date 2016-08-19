/*

 libCWebUI
 Copyright (C) 2007 - 2016  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

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

