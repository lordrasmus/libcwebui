/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/


#ifndef _WS_HELPER_H_
#define _WS_HELPER_H_



#if __GNUC__ > 2
		#define VISIBLE_ATTR __attribute__ ((visibility("default")))
        #define NEED_RESUL_CHECK __attribute__((warn_unused_result))
	
		#ifndef likely
			#define likely(x)       __builtin_expect((x),1)
		#endif
		#ifndef unlikely
			#define unlikely(x)     __builtin_expect((x),0)
		#endif

		#define UNUSED_PARA __attribute__((unused))
#else
		#define VISIBLE_ATTR
		#define NEED_RESUL_CHECK
		
		#define likely(x)      x
		#define unlikely(x)    x

		#define UNUSED_PARA
#endif



#ifdef USE_ASSERTION

	#define WS_ASSERT( a ) if ( a ) { \
		printf("\nASSERT( "#a" ) -> File: "__FILE__"  Func: %s  Line: %d\n",__PRETTY_FUNCTION__,__LINE__); \
		fflush(stdout); \
		raise(SIGTRAP); \
	}

#else

	#define WS_ASSERT( a ) { }

#endif

#endif
