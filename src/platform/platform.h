#ifndef PLATFORM_H_
#define PLATFORM_H_ 1

/*
   The platform layer.
*/

#include "../base/base_defs.h"
#include "../base/base_string.h"

//#include <stddef.h>

typedef struct HeapSystemInfo {
    u32 logicProcessorCount;
    u64 pageSize;
    u64 largePageSize;
    u64 allocGranularity;
} HeapSystemInfo;


extern void PlatformSleepMs(unsigned int ms);
extern u64 PlatformTimeUsec(void);

extern void HeapGetSystemInfo(HeapSystemInfo** out);

extern void* HeapReserve(size_t size);
extern int HeapRelease(void* ptr, size_t size);

extern int HeapCommit(void* ptr, size_t size);
extern int HeapDecommit(void* ptr, size_t size);


extern void PlatformInit(void);
extern void PlatformTerminate(void);

extern u32 FastPrint(String8 str);
extern void Platform_fatalError(const char* msg, int code);

extern int Platform_fileRead(void* args, String8 path, u8** data, u32* size);

#endif /* PLATFORM_H_ */
