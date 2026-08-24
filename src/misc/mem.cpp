#include "mem.h"
#include "stack.h"
#include <sysinfoapi.h>

static HEAP_SYSTEMINFO os_info = { 0 };
static int is_cached = 0;

static STACKALLOC largeStack;

#if HAS_WINDOWS
#include <windows.h>

extern void HeapGetSystemInfo(HEAP_SYSTEMINFO **out)
{
    if (!is_cached)
    {
        SYSTEM_INFO sys_info = { 0 };
        GetSystemInfo(&sys_info);

        os_info.logical_processor_count = (uint32_t) sys_info.dwNumberOfProcessors;
        os_info.page_size = (uint64_t) KB(4);
        os_info.large_page_size = MB(2);
        os_info.allocation_granularity = os_info.page_size;

        is_cached = 1;
    }

    *out = &os_info;
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

#else

/*
   Assume it's an UNIX system with POSIX calls.
*/

#include <sys/mman.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/sysinfo.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>


extern void HeapGetSystemInfo(HEAP_SYSTEMINFO **out)
{
    if (!is_cached)
    {
#if HAS_ANDROID
        os_info.logical_processor_count = sysconf(_SC_NPROCESSORS_ONLN);
        os_info.page_size = (uint64_t) getpagesize();
        os_info.large_page_size = MB(2);
        os_info.allocation_granularity = os_info.page_size;
#elif HAS_LINUX
        os_info.logical_processor_count = (uint32_t) get_nprocs();
        os_info.page_size = (uint64_t) getpagesize();
        os_info.large_page_size = MB(2);
        os_info.allocation_granularity = os_info.page_size;
#endif
        is_cached = 1;
    }

    *out = &os_info;
}

// TODO: Fix.

extern void* HeapReserve(size_t size)
{
    void *result = mmap(0, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (result == MAP_FAILED)
    {
        result = 0;
    }
    return result;
}

extern int HeapRelease(void* ptr, size_t size)
{
    return munmap(ptr, size);
}

extern int HeapCommit(void* ptr, size_t size)
{
    int res = mprotect(ptr, size, PROT_READ | PROT_WRITE);
    if (res == 1)
    {
        fprintf(stderr, "mprotect failed to commit memory: %s", strerror(errno));
        *((volatile char *) 0x00) = 0xff;
    }
    return ret;
}
extern int HeapDecommit(void* ptr, size_t size)
{
    madvise(ptr, size, MADV_DONTNEED);
    return mprotect(ptr, size, PROT_NONE);
}

#endif

#define LARGE_STACK_SIZE MB(16)

extern int MemoryInit(void)
{
    size_t reserve_size = LARGE_STACK_SIZE;
    size_t commit_size = LARGE_STACK_SIZE >> 1;

    HEAP_SYSTEMINFO* sysinfo;
    HeapGetSystemInfo(&sysinfo);

    reserve_size = ALIGN_UP_POW2(reserve_size, sysinfo->page_size);
    commit_size = ALIGN_UP_POW2(commit_size, sysinfo->page_size);

    void* backing = HeapReserve(reserve_size);
    HeapCommit(backing, commit_size);

    if (!backing)
    {
        fprintf(stderr, "%s: Failed to initialize memory!\n", __func__);
        *((volatile char *) 0x00) = 0xFF;
    }

    // Now initialize the stack.
    StackInit(&largeStack, backing, reserve_size);

    return 0;
}

extern void MemoryTerminate(void)
{
    void* backing = largeStack.buf;
    HeapRelease(backing, largeStack.reservedSize);
}

// TODO: The naming, though.
extern void* StackDoAlloc(size_t bytes)
{
    return StackAlloc(&largeStack, bytes);
}
extern void StackDoDealloc(void* ptr)
{
    StackFree(&largeStack, ptr);
}

