/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/


#ifndef _PLATFORM_FILE_ACCESS_H_
#define _PLATFORM_FILE_ACCESS_H_



/********************************************************************
*																	*
*					Datei Operationen     							*
*																	*
********************************************************************/

char			WebServerReadDataStart( void );
int 			PlatformOpenDataReadStream( const char* name );
int				PlatformGetFileSize(void);
int 			PlatformGetFileInfo(WebserverFileInfo* file, int* time_changed, int *new_size);
char			PlatformCloseDataStream( void );
void			PlatformResetDataStream( void );
unsigned long	PlatformGetDataStreamPosition( void );
void  			PlatformSeek(long offset);
void  			PlatformSeekToPosition( long position );
int				PlatformReadBytes(unsigned char *data, FILE_OFFSET lenght);

void  			PlatformPrintPosition( void );



#endif

