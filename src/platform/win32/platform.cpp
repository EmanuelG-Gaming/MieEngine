#include "platform.h"
#include "../platform.h"

#include "../../base/base_log.h"
#include "../../base/base_string.h"

// Doing these direct things.
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

#include <stdio.h>



static HeapSystemInfo osInfo = { 0 };
static int isCached = 0;

static HANDLE logFile = INVALID_HANDLE_VALUE;

static u64 perfFreq = 1;


extern void PlatformInit(void)
{
    logFile = GetStdHandle(STD_OUTPUT_HANDLE);
    // Performance frequency.
    LARGE_INTEGER pFreq = { 0 };
    if (QueryPerformanceFrequency(&pFreq))
    {
        perfFreq = (u64) pFreq.QuadPart;
    }
    /*
    else
    {
        fprintf(stderr, "%s: Failed to query performance frequency!\n", __func__);
        return -1;
    }
    */

}

extern void PlatformTerminate(void)
{
    if (logFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(logFile);
    }
}


extern void PlatformSleepMs(unsigned int ms)
{
    Sleep(ms);
}
extern u64 PlatformTimeUsec(void)
{
    LARGE_INTEGER ticks = { 0 };

    if (!QueryPerformanceCounter(&ticks))
    {
        //fprintf(stderr, "%s: Failed to query performance counter!\n", __func__);
        return 0;
    }

    return (u64) ticks.QuadPart * 1000000 / perfFreq;
}


extern void HeapGetSystemInfo(HeapSystemInfo **out)
{
    if (!isCached)
    {
        SYSTEM_INFO sys_info = { 0 };
        GetSystemInfo(&sys_info);

        osInfo.logicProcessorCount = (uint32_t) sys_info.dwNumberOfProcessors;
        osInfo.pageSize = (uint64_t) KB(4);
        osInfo.largePageSize = MB(2);
        osInfo.allocGranularity = osInfo.pageSize;

        isCached = 1;
    }

    *out = &osInfo;
}

extern void* HeapReserve(size_t size)
{
    return VirtualAlloc(0, size, MEM_RESERVE, PAGE_READWRITE);
}
extern int HeapRelease(void* ptr, size_t size)
{
    UNUSED(size);
    return !VirtualFree(ptr, 0, MEM_RELEASE);
}

extern int HeapCommit(void* ptr, size_t size)
{
    void* ret = VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE);
    return ret == NULL;
}
extern int HeapDecommit(void* ptr, size_t size)
{
    return !VirtualFree(ptr, size, MEM_DECOMMIT);
}



/*
   Printing.
*/

extern u32 FastPrint(String8 str)
{
    DWORD bytesWritten;
    WriteFile(logFile, (const char *) str.data, (int) str.len, &bytesWritten, NULL);

    return (u32) bytesWritten;
}


extern void Platform_fatalError(const char *msg, int code)
{
    MessageBoxA(NULL, msg, "Error", MB_OK | MB_ICONERROR);
    ExitProcess((u32) code);
}

extern int Platform_fileRead(void* args, String8 path, u8** data, u32* size)
{
    ARENA_TEMP scratchArena = ArenaScratchGet(NULL, 0);

    String16 path16 = Str16FromStr8(scratchArena.arena, path, TRUE);
    fprintf(stderr, "path size: %zu\n", path.len);

    HANDLE fileHandle = CreateFileW(
        (LPCWSTR) path16.data,
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    ArenaScratchRelease(scratchArena);

    if (fileHandle == INVALID_HANDLE_VALUE)
    {
        Platform_fatalError("Failed to open file!", -1);
        return -1;
    }

    String8 out = { 0 };
    DWORD highSize = 0;
    DWORD lowSize = GetFileSize(fileHandle, &highSize);
    u64 totalSize = ((u64) highSize << 32) | lowSize;

    u8* buffer;
    ARENA_TEMP possibleTemp = { 0 };

    if (args)
    {
        ARENA* arena = reinterpret_cast<ARENA *> (args);
        possibleTemp = ArenaTempBegin(arena);
        buffer = (u8 *) ArenaPush(arena, sizeof(u8) * totalSize, 0);
    }
    else
    {
        buffer = (u8 *) _MALLOC(sizeof(u8) * totalSize);
    }

    u64 totalRead = 0;
    while (totalRead < totalSize)
    {
        // Read the file in chunks.
        u64 toRead64 = totalSize - totalRead;
        DWORD toRead = toRead64 > ~(DWORD)(0) ? ~(DWORD)(0) : (DWORD) toRead64;

        DWORD bytesRead = 0;
        if (ReadFile(fileHandle, buffer + totalRead, toRead, &bytesRead, 0) == FALSE)
        {
            if (args)
            {
                ArenaTempEnd(possibleTemp);
            }
            else
            {
                _FREE(buffer);
                buffer = NULL;
            }
            Platform_fatalError("Failed to read file!", -1);

            return -1;
        }

        totalRead += bytesRead;
    }

    if (data) *data = buffer;
    if (size) *size = totalSize;

    CloseHandle(fileHandle);

    return 0;
}


