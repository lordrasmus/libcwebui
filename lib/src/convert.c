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

#include "stdafx.h"

#ifdef __GNUC__	
	
#endif


unsigned int getInt(unsigned char *buffer)
{
    unsigned int tmp=0;
    tmp+=buffer[0];
    tmp+=buffer[1]<<8;
    tmp+=buffer[2]<<16;
    tmp+=buffer[3]<<24;
    return tmp;
}


/*void getIPByteFromString(unsigned char *ipbytes,unsigned char *string){
   int i2,dez;
   unsigned char tmp;
   size_t length = strlen((char*)string);
   size_t i;

   dez=1;
   i2=3;
   tmp=0;

   for( i = length - 1 ; i > 0 ; i--){
	if(string[i]=='.'){
		ipbytes[i2--]=tmp;
		dez=1;
		tmp=0;
 		continue;
	}
 	tmp += (unsigned char) ( ( string[i] - '0' ) * dez );
 	dez*=10;

     // iprintf("Z : %d (%c) %d\n",string[i]-'0',string[i],tmp);      
   }

   ipbytes[0]=tmp;

}*/
