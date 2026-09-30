#include "../platform.h"

#include <sys/mman.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/sysinfo.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

static HeapSystemInfo osInfo = { 0 };
static int isCached = 0;


extern void HeapGetSystemInfo(HeapSystemInfo **out)
{
    if (!isCached)
    {
#if HAS_ANDROID
        os_info.logicProcessorCount = sysconf(_SC_NPROCESSORS_ONLN);
        os_info.pageSize = (uint64_t) getpagesize();
        os_info.largePageSize = MB(2);
        os_info.allocGranularity = os_info.page_size;
#elif HAS_LINUX
        os_info.logicProcessorCount = (uint32_t) get_nprocs();
        os_info.pageSize = (uint64_t) getpagesize();
        os_info.largePageSize = MB(2);
        os_info.allocGranularity = os_info.page_size;
#endif
        isCached = 1;
    }

    *out = &osInfo;
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

