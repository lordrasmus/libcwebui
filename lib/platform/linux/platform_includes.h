
#ifndef _PLATFORM_INCLUDES_H_
#define _PLATFORM_INCLUDES_H_

#if defined( LINUX ) || defined ( OSX )

#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <dlfcn.h>
#include <pthread.h>
#include <semaphore.h>

#if defined( LINUX )
	#include <sys/sendfile.h>
#endif

#include <strings.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#include <netinet/in.h>

#include <sys/select.h>
#include <sys/ioctl.h>


#if defined( LINUX )
	#define FILE_OFFSET __off_t
#endif

#if defined( OSX )
	#define FILE_OFFSET __darwin_off_t
#endif



#define WS_MUTEX_TYPE		pthread_mutex_t
#define WS_SEMAPHORE_TYPE	sem_t
#define WS_THREAD			pthread_t

#define WebServerPrintf printf

#endif

#endif
