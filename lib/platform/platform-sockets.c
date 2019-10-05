
#include "include/platform-defines.h"

#ifdef LINUX
	#include "linux/platform-sockets.c"
#endif

#ifdef OSX
	#include "osx/platform-sockets.c"
#endif
