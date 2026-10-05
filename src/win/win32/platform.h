#ifndef PLAT_H_
#define PLAT_H_ 1

/*
   NOTE: Duct-taped solution, maybe>
*/

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE

#include <windows.h>
#include <minwinbase.h>

//#include <d3d11.h>
//#include <d3dcompiler.h>
#endif /* _WIN32 */


#if _MSC_VER
#pragma comment(lib, "uuid")
#pragma comment(lib, "dxguid")
#pragma comment(lib, "d3dcompiler")
#pragma comment(lib, "dxgi")

#pragma comment(lib, "d3d9")
#pragma comment(lib, "d3d11")

#pragma comment(lib, "windowscodecs.lib")
#endif

#endif /* PLAT_H_ */
