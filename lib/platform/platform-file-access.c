
#include "include/platform-defines.h"

#ifdef LINUX
	#include "linux/platform-file-access.c"
#endif

#ifdef OSX
	#include "osx/platform-file-access.c"
#endif

#ifdef _WIN32
	#include "win/platform-file-access.c"
#endif