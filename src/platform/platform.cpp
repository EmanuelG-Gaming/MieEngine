#include "platform.h"
#include "../base/base_defs.h"



// Classic unity build.
#if defined(HAS_WINDOWS)
#include "win32/platform.cpp"
#else
#include "linux/platform.cpp"
#endif
