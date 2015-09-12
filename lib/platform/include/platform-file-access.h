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


#ifndef _PLATFORM_FILE_ACCESS_H_
#define _PLATFORM_FILE_ACCESS_H_



/********************************************************************
*																	*
*					Datei Operationen     							*
*																	*
********************************************************************/

char			WebServerReadDataStart( void );
int 			PlatformOpenDataReadStream( char* name );
int				PlatformGetFileSize(void);
int 			PlatformGetFileInfo(WebserverFileInfo* file, int* time_changed, int *new_size);
char			PlatformCloseDataStream( void );
void			PlatformResetDataStream( void );
unsigned long	PlatformGetDataStreamPosition( void );
void  			PlatformSeek(long offset);
void  			PlatformSeekToPosition( long position );
int				PlatformReadBytes(unsigned char *data,FILE_OFFSET lenght);

void  			PlatformPrintPosition( void );



#endif

