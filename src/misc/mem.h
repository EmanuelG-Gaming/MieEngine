#ifndef MEM_H_
#define MEM_H_ 1

#include "../base/base_defs.h"


typedef struct HEAP_SYSTEMINFO {
    u32 logical_processor_count;
    u64 page_size;
    u64 large_page_size;
    u64 allocation_granularity;
} HEAP_SYSTEMINFO;

typedef struct PLATFORMHEAP {
    void* alloc_address;
    size_t alloc_size;
} PLATFORMHEAP;


extern void HeapGetSystemInfo(HEAP_SYSTEMINFO** out);

extern void* HeapReserve(size_t size);
extern int HeapRelease(void* ptr, size_t size);
extern int HeapCommit(void* ptr, size_t size);
extern int HeapDecommit(void* ptr, size_t size);

/*
   Global.
*/

extern int MemoryInit(void);
extern void MemoryTerminate(void);

extern void* StackDoAlloc(size_t bytes);
extern void StackDoDealloc(void* ptr);


#endif /* MEM_H_ */
