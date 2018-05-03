
#pragma once



#include <ws2tcpip.h>
#include <tchar.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <winsock2.h>
#include <strsafe.h>
#include <io.h>

#include "webserver.h"


#pragma warning(disable:4996) // warning C4996: strncpy': This function or variable may be unsafe. Consider using strncpy_s instead.

#pragma warning(disable:4267) // warning C4267: '=': Konvertierung von 'size_t' nach 'int', Datenverlust mglich
#pragma warning(disable:4244) // warning C4244: 'return': Konvertierung von '__w64 int' in 'int', mglicher Datenverlust
#pragma warning(disable:4476) // warning C4476 : "printf" : Unbekanntes Typfeldzeichen "m" im Formatbezeichner.
#pragma warning(disable:4477) // warning C4477 : "printf" : Die Formatzeichenfolge "%lu" erfordert ein Argument vom Typ "unsigned long", das variadic - Argument "1" weist aber den Typ "uint64_t" auf.



#define WS_MUTEX_TYPE		HANDLE
#define WS_SEMAPHORE_TYPE	HANDLE 
#define WS_THREAD			HANDLE
#define WebServerPrintf		printf
#define FILE_OFFSET			uint64_t

int strcasecmp(const char *s1, const char *s2);