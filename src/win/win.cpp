#include "base/base_defs.h"

#if defined(HAS_WINDOWS)
#include "win32/window.cpp"
#else
#error "UNIX, Linux not implemented!"
#endif
