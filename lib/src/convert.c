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

#ifdef DMALLOC
#include <dmalloc/dmalloc.h>
#endif


unsigned int getInt(const unsigned char *buffer)
{
    unsigned int tmp=0;
    tmp+=buffer[0];
    tmp+=buffer[1]<<8;
    tmp+=buffer[2]<<16;
    tmp+=buffer[3]<<24;
    return tmp;
}

unsigned char toHex(unsigned char in) {
	if ((in <= '9') && (in >= '0')) {
		return (unsigned char)(in - '0');
	}

	if ((in >= 'a') && (in <= 'f')) {
		return (unsigned char)(in - 87);
	}

	if ((in >= 'A') && (in <= 'F')) {
		return (unsigned char)(in - 55 );
	}

	return 0;
}


void url_decode(char *line) {
	unsigned char hex;
	size_t i_in=0,i_out=0;
	size_t lenght;

	lenght = strlen((char*) line);
	for (i_in = 0; i_in <= lenght; i_in++) {
		if ( (unlikely(line[i_in]=='%')) && ( ( lenght - i_in ) > 2 ) )  {
			hex = (unsigned char)(toHex(line[i_in + 1]) << 4);
			hex =  (unsigned char)( hex + toHex(line[i_in + 2]) );
			/*  hexcode des zeichens als char speichern und 2 zeichen loeschen */
			line[i_out] = hex; 
			i_out++;
			i_in+=2;
			continue;
		}
		if ( unlikely(line[i_in] == '+') ){ /* + durch whitespace ersetzen */
			line[i_out] = ' ';
			i_out++;
			continue;
		}
		line[i_out] = line[i_in];
		i_out++;
	}
	
}


