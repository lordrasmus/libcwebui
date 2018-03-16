
#ifndef _PLATFORM_INCLUDES_H_
#define _PLATFORM_INCLUDES_H_

#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

#include <pthread.h>
#include <semaphore.h>

#include <netinet/in.h>

#include <sys/select.h>
#include <sys/ioctl.h>


/*
#include <dlfcn.h>
*/

#define WS_MUTEX_TYPE pthread_mutex_t
#define WS_SEMAPHORE_TYPE sem_t

#define WebServerPrintf printf


#endif
