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

