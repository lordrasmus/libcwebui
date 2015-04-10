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


#include "webserver.h"


void StringDest( UNUSED_PARA void* a) {
	/* free((int*) a); */
}

int StringComp(const void* a, const void* b) {
	int ret;
	char *p_a = (char*) a;
	char *p_b = (char*) b;

	if ( ( p_a == 0 ) || ( p_b == 0 ) ){
		return 0;
	}

	ret = strcmp(p_a, p_b);
	if (ret < 0) return -1;
	if (ret > 0) return 1;
	return 0;
}

void StringPrint(const void* a) {
	printf("%s", (char*) a);
}

void StringInfoPrint( UNUSED_PARA void* a) {
}

void StringInfoDest( UNUSED_PARA void *a) {
}

rb_red_blk_tree* initStringRBTree(void) {
	return RBTreeCreate(StringComp, StringDest, StringInfoDest, StringPrint, StringInfoPrint);
}

unsigned int readInt(void) {
	unsigned char intbuffer[4];
	unsigned int tmp;
	PlatformReadBytes(intbuffer, 4);
	tmp  = ((unsigned int)intbuffer[0]) << 0;
	tmp += ((unsigned int)intbuffer[1]) << 8;
	tmp += ((unsigned int)intbuffer[2]) << 16;
	tmp += ((unsigned int)intbuffer[3]) << 24;
	return tmp;
}


int stringfind(const char *buffer, const char *pattern) {
	int length;
	int i, i2 = 0;
	if ( buffer == 0){
		return 0;
	}
	length = (int) strlen((char*) buffer);
	for (i = 0; i < length; i++)
		if (buffer[i] == pattern[i2]) {
			i2++;
			if (pattern[i2] == '\0') return i;
		} else {
			i2 = 0;
		}
	return 0;

}

long stringnfind(const char *buffer, const char *pattern, unsigned int buffer_length) {
	unsigned int i, i2 = 0;
	
	if ( ( buffer == 0 ) || ( pattern == 0 ) ){
		return -1;
	}
	
	for (i = 0; (i < buffer_length); i++){
		if (buffer[i] == pattern[i2]) {
			i2++;
			if (pattern[i2] == '\0'){
				 return i;
			}
		} else {
			i2 = 0;
		}
	}
	return -1;
}

int stringfindAtPos(const char *buffer, const char *pattern) {
	int i, i2 = 0;
	int lenght = (int) strlen((char*) pattern);
	for (i = 0; i < lenght; i++)
		if (buffer[i] == pattern[i2]) {
			i2++;
			if (pattern[i2] == '\0') return i;
			if (i2 == lenght) return i;
		} else {
			return 0;
		}
	return 0;

}

void Webserver_strncpy(char *dest, unsigned int dest_size, const char *src, unsigned int count) {
	if ( ( dest == 0 ) || ( src == 0 ) ) {
		return;
	}
#ifdef _MSC_VER
#define Webserver_strncpy_ok
	strncpy_s(dest,dest_size,src,count);
#endif
#ifdef __GNUC__
#define Webserver_strncpy_ok
	if ( dest_size >= count){
		strncpy(dest, src, count);
	}else{
		strncpy(dest, src, dest_size);
	}
#endif
#ifndef Webserver_strncpy_ok
#error not implemented
#endif
	if ( dest_size >= 1 ){
		dest[dest_size-1] = '\0';
	}
}

unsigned long Webserver_strlen(char *text) {
	return strlen(text);
}

void getHTMLMonth(unsigned char month, char* buffer, SIZE_TYPE size) {
	switch (month) {
	case 1:
		snprintf((char*) buffer, size, "Jan");
		break;
	case 2:
		snprintf((char*) buffer, size, "Feb");
		break;
	case 3:
		snprintf((char*) buffer, size, "Mar");
		break;
	case 4:
		snprintf((char*) buffer, size, "Apr");
		break;
	case 5:
		snprintf((char*) buffer, size, "May");
		break;
	case 6:
		snprintf((char*) buffer, size, "Jun");
		break;
	case 7:
		snprintf((char*) buffer, size, "Jul");
		break;
	case 8:
		snprintf((char*) buffer, size, "Aug");
		break;
	case 9:
		snprintf((char*) buffer, size, "Sep");
		break;
	case 10:
		snprintf((char*) buffer, size, "Oct");
		break;
	case 11:
		snprintf((char*) buffer, size, "Nov");
		break;
	case 12:
		snprintf((char*) buffer, size, "Dec");
		break;
	default:
		snprintf((char*) buffer, size, "Unknown Month %d", month);
		break;
	}
}

/* HTML date  format http://www.hackcraft.net/web/datetime/#rfc822} */

unsigned int getHTMLDateFormat(char* buffer, int day, int month, int year, int hour, int minute) {
	char bb[30];

	getHTMLMonth(month, bb, 30);

	return snprintf(buffer, 1000, "%02d %s %04d %02d:%02d GMT", day, bb, year, hour, minute);

}

void convertBinToHexString(unsigned char* bin, int bin_length, char* text, int text_length) {
	int l;
	for (l = 0; l < bin_length; l++) {
		text[l * 2 + 0] = bin[l] >> 4;
		if (text[l * 2 + 0] <= 9)
			text[l * 2 + 0] += 48;
		else
			text[l * 2 + 0] += 55;
		text[l * 2 + 1] = bin[l] & 0x0F;
		if (text[l * 2 + 1] <= 9)
			text[l * 2 + 1] += 48;
		else
			text[l * 2 + 1] += 55;
	}
	text[text_length] = '\0';
}

void generateGUID(char* buf, int length) {
#ifdef WEBSERVER_USE_SSL
	int l;
	unsigned char *tmp = (unsigned char*) WebserverMalloc ( length / 2 );
	if (0 != WebserverRANDBytes(tmp, length / 2)) {
		convertBinToHexString(tmp, length / 2, buf, length);
		l = strlen(buf);
		if (l != length)
		LOG( PLATFORM_LOG, NOTICE_LEVEL, 0, "error generating Data %d->%d %s",length,l, buf);
		WebserverFree(tmp);
		return;
	}
	WebserverFree(tmp);
#endif

	PlatformGetGUID(buf, length);
}

