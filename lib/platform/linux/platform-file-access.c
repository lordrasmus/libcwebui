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


#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

#include "webserver.h"


#ifdef DMALLOC
#include <dmalloc/dmalloc.h>
#endif

static FILE *g_fp;
unsigned short l;


/**************************************************************
 *                                                             *
 *                   Datei Operationen                         *
 *                                                             *
 **************************************************************/

int PlatformOpenDataReadStream( const unsigned char* name ) {
	struct stat st;

	g_fp = fopen( ( char* ) name, "rb");
	if (g_fp == NULL) return false;

	/* Auf normale Datei Prüfen */
	fstat(fileno(g_fp), &st);
	if (!S_ISREG(st.st_mode)) return false;

	return fileno(g_fp);
}


int PlatformGetFileSize(void) {
	struct stat st;
	fstat(fileno(g_fp), &st);
	return st.st_size;
}

char PlatformCloseDataStream(void) {
	fclose(g_fp);
	return true;
}

void PlatformResetDataStream(void) {
	fseek(g_fp, 0, SEEK_SET); /* Anfang der Datei */
}

int PlatformReadBytes(unsigned char *data, FILE_OFFSET lenght) {
	return fread(data, 1, lenght, g_fp);
}

void PlatformSeek(long offset) {
	if ( 0 != fseek(g_fp, offset, SEEK_CUR) )
		printf("fseek error : %m\n");
}

void PlatformSeekToPosition(long position) {
	if ( 0 != fseek(g_fp, position, SEEK_SET) )
		printf("fseek error : %m\n");
}

/*long WebserverGetDataStreamPosition(void) {
	long ret = ftell(g_fp);
	if ( ret < 0 ){
		printf("WebserverGetDataStreamPosition: ret ( %d ) < 0 \n",ret );
	}
	return ret;
}*/

/*	 struct stat {
 off_t     st_size;    // total size, in bytes
 time_t    st_atime;   // time of last access
 time_t    st_mtime;   // time of last modification
 time_t    st_ctime;   // time of last status change
 };
 */

int PlatformGetFileInfo(WebserverFileInfo* file, int* time_changed, int *new_size) {

	struct stat st;
	struct tm *ts;
	unsigned int len;
	char* buffer;
	__time_t f_sec,f_nsec;

	if ( 0 > stat( (char*) file->FilePath, &st) ){
		return 0;
	}

#ifdef __USE_MISC
	f_sec = st.st_mtim.tv_sec;
	f_nsec = st.st_mtim.tv_nsec;
#else
	f_sec = st.st_mtime;
/*	f_nsec = st.st_atimensec;*/
	f_nsec = st.st_atime;
#endif

	*new_size = st.st_size;
	*time_changed = 0;

	/* st.st_mtim.tv_nsec; */
	if ( ( file->last_mod_sec != (unsigned long int)f_sec) || ( file->last_mod_nsec != (unsigned long int)f_nsec) ){

		*time_changed = 1;

		file->last_mod_sec = f_sec;
		file->last_mod_nsec = f_nsec;



		buffer = (char *) WebserverMalloc( 150 );
		ts = localtime(&st.st_mtime);
		len = getHTMLDateFormat(buffer, ts->tm_mday, ts->tm_mon, ts->tm_year + 1900, ts->tm_hour, ts->tm_min);

		if (file->lastmodified == 0) {
			file->lastmodified = (char *) WebserverMalloc( len + 1 );
			memcpy(file->lastmodified, buffer, len + 1);
			file->lastmodifiedLength = len;
			WebserverFree(buffer);
			return true;
		} else {
			if (0 != strcmp(file->lastmodified, buffer)) {
				WebserverFree(file->lastmodified);
				file->lastmodified = (char *) WebserverMalloc( len + 1 );
				memcpy(file->lastmodified, buffer, len + 1);
				file->lastmodifiedLength = len;
				WebserverFree(buffer);
				return true;
			}
		}
		WebserverFree(buffer);
	}

	return 1;
}

