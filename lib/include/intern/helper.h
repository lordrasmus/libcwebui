
#ifndef _WS_HELPER_H_
#define _WS_HELPER_H_

#include <signal.h>

#define UNUSED_PARA __attribute__((unused))
#define NEED_RESUL_CHECK __attribute__((warn_unused_result))


#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)


#define FILE_OFFSET __off_t
#define SIZE_TYPE size_t
#define TIME_TYPE time_t

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
