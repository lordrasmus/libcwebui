
#ifndef _PLATFORM_INCLUDES_H_
#define _PLATFORM_INCLUDES_H_

#ifdef LINUX

#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <dlfcn.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/sendfile.h>

#include <strings.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#include <netinet/in.h>

#include <sys/select.h>
#include <sys/ioctl.h>

#define SIZE_TYPE size_t
#define TIME_TYPE time_t

#define FILE_OFFSET __off_t

#define WS_MUTEX_TYPE		pthread_mutex_t
#define WS_SEMAPHORE_TYPE	sem_t
#define WS_THREAD			pthread_t

#define WebServerPrintf printf

#endif

#endif
