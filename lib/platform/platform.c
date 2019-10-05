
#include "include/platform-defines.h"

#ifdef LINUX
	#include "linux/system-unix.c"
#endif

#ifdef OSX
	#include "osx/system-unix.c"
#endif
