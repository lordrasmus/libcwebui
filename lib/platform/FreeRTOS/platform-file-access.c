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


#include "webserver.h"


/**************************************************************
 *                                                             *
 *                   Datei Operationen                         *
 *                                                             *
 **************************************************************/

int PlatformOpenDataReadStream( const char* name ) {
	return 0;
}


int PlatformGetFileSize(void) {
	return 0;
}

char PlatformCloseDataStream(void) {
	return 0;
}

void PlatformResetDataStream(void) {
	
}

int PlatformReadBytes(unsigned char *data, FILE_OFFSET lenght) {
	return 0;
}

void PlatformSeek(long offset) {
	
}

void PlatformSeekToPosition(long position) {
	
}


int PlatformGetFileInfo(WebserverFileInfo* file, int* time_changed, int *new_size) {

	return 1;
}

