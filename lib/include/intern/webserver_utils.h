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


#ifndef _WEBSERVER_UTILS_H_
#define _WEBSERVER_UTILS_H_

//#include "../WebserverConfig.h"
#include "red_black_tree.h"

#ifdef __cplusplus
extern "C" {
#endif

rb_red_blk_tree* initStringRBTree(void);

unsigned int readInt( void );

//void writeInt( unsigned int data );

int stringfind(const char *buffer,const char *pattern);
long stringnfind(const char *buffer,const char *pattern,unsigned int length);

int stringfindAtPos(const char *buffer,const char *pattern);

void Webserver_strncpy( char *dest, unsigned int dest_size,  const char *src, unsigned int count);
unsigned long Webserver_strlen( char *text);



unsigned int getHTMLDateFormat( char* buffer,int day,int month,int year,int hour,int minute);

// int pos=0; in der funktion definieren zum benutzen, in pos steht dann die string l���nge
#define BufferPrint(...) pos+=snprintf((char*)retBuffer+pos,ret_length-pos, __VA_ARGS__)

void 	generateGUID ( char* buf,int length );

void 	convertBinToHexString(unsigned char* bin,int bin_length,char* text, int text_length);



#ifdef __cplusplus
}
#endif

#endif
