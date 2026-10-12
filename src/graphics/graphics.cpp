#include "../base/base_defs.h"

#define GFX_USE_DX11

/*
#ifdef HAS_LINUX
#undef GFX_USE_DX9
#undef GFX_USE_DX11

#define GFX_USE_OPENGL
#endif
*/

#if defined(GFX_USE_DX9)
#include "dx9/graphics.cpp"
#elif defined(GFX_USE_DX11)
#include "dx11/graphics.cpp"
#elif defined(GFX_USE_OPENGL)
#include "opengl/graphics.cpp"
#endif
