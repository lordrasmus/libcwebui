/*
 * Based on Code written by Manuel Badzong.
 * 

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
 * 
 */


#include "webserver.h"

static char encoder[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


static int
base64_encsize(int size)
{
	return 4 * ((size + 2) / 3);
}

static int
base64_encode(char *dest, int size, const unsigned char *src, int slen)
{
	int dlen, i, j;
	uint32_t a, b, c, triple;

	dlen = base64_encsize(slen);

	// Sanity checks
	if (src == NULL || dest == NULL)
	{
		return -1;
	}
	if (dlen + 1 > size)
	{
		return -1;
	}
	if (slen == 0)
	{
		if (size > 0)
		{
			dest[0] = 0;
			return 0;
		}
		return -1;
	}

	for (i = 0, j = 0; i < slen;)
	{
		a = src[i++];

		// b and c may be off limit
		b = i < slen ? src[i++] : 0;
		c = i < slen ? src[i++] : 0;

		triple = (a << 16) + (b << 8) + c;

		dest[j++] = encoder[(triple >> 18) & 0x3F];
		dest[j++] = encoder[(triple >> 12) & 0x3F];
		dest[j++] = encoder[(triple >> 6) & 0x3F];
		dest[j++] = encoder[triple & 0x3F];
	}

	// Pad zeroes at the end
	switch (slen % 3)
	{
	case 1:
		dest[j - 2] = '=';
		// fall through
	case 2:
		dest[j - 1] = '=';
	}

	// Always add \0
	dest[j] = 0;

	return dlen;
}


void WebserverBase64Encode(const unsigned char *input, int length, unsigned char *output, SIZE_TYPE out_length) {
	memset( output, 0 , out_length );
	
	base64_encode( (char*)output, out_length, input, length );
}


