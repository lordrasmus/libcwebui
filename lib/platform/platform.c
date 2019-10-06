
#include "include/platform-defines.h"

#ifdef LINUX
	#include "linux/system-unix.c"
#endif

#ifdef OSX
	#include "osx/system-unix.c"
#endif

#ifdef _WIN32
	#include "win/platform-win.c"
#endif