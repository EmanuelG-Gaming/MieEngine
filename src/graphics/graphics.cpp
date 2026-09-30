#define GFX_USE_DX9

#if defined(GFX_USE_DX9)
#include "dx9/graphics.cpp"
#elif defined(GFX_USE_DX11)
#include "dx11/graphics.cpp"
#endif
