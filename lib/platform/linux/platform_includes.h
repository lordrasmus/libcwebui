/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/



#ifndef _PLATFORM_INCLUDES_H_
#define _PLATFORM_INCLUDES_H_

#ifdef LINUX 
	#error LINUX already define remove
#endif

#define LINUX


#ifndef _DEFAULT_SOURCE
	#define _DEFAULT_SOURCE
#endif

#include <stdio.h>
#include <ctype.h>
#include <dlfcn.h>
#include <errno.h>

#include <limits.h>
#include <unistd.h>

#include <inttypes.h>

#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>

#include <sys/sendfile.h>

#include <string.h>
#include <strings.h>

#include <sys/types.h>
#include <unistd.h>

#include <netinet/in.h>

#include <sys/ioctl.h>
#include <sys/select.h>


#define SIZE_TYPE size_t
#define SIZE_TYPE_PRINT_DEZ "zu"

#define TIME_TYPE time_t

#ifdef __MUSL__
	#define FILE_OFFSET off_t
#else
	#define FILE_OFFSET __off_t
#endif
	
#define FILE_OFF_PRINT_HEX "jX"
#define FILE_OFF_PRINT_INT "lu"


#define WS_MUTEX_TYPE		pthread_mutex_t
#define WS_SEMAPHORE_TYPE	sem_t
#define WS_THREAD			pthread_t

#define WebServerPrintf printf



#endif
