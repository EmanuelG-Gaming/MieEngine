#ifndef ARENA_H_
#define ARENA_H_ 1

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS (1)
#endif



#if defined(__clang__)
#define ARENA_COMPILER_CLANG
#elif defined(__GNUC__) || defined(__GNUG__)
#define ARENA_COMPILER_GCC
#elif defined(_MSC_VER)
#define ARENA_COMPILER_MSVC
#else
#define ARENA_COMPILER_UNKNOWN
#endif

#if defined(ARENA_COMPILER_CLANG) || defined(ARENA_COMPILER_GCC)
#define ARENA_THREAD_LOCAL __thread
#elif defined(ARENA_COMPILER_MSVC)
#define ARENA_THREAD_LOCAL __declspec(thread)
#elif (__STDC_VERSION__ >= 201112L)
#define ARENA_THREAD_LOCAL _Thread_local
#else
#error "Invalid compiler/version for thread variable. Use Clang, GCC, MSVC, or use C11 or greater."
#endif


#ifndef ARENA_MEMSET
#include <string.h>
#define ARENA_MEMSET memset
#endif

#if defined(_WIN32)
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/sysinfo.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#endif

#include <stdint.h>




#define ARENA_UNUSED(expr) (void) expr

#ifdef __cplusplus
#define CLITERAL(T) T
#else
#define CLITERAL(T) (T)
#endif


#define ARENA_ALIGN_UP_POW2(n, p) (((n) + ((p) - 1)) & (~((p) - 1)))

#define KB(n) (((uint64_t)(n)) << 10)
#define MB(n) (((uint64_t)(n)) << 20)
#define GB(n) (((uint64_t)(n)) << 30)
#define TB(n) (((uint64_t)(n)) << 40)


#define ARENA_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define ARENA_MAX(a, b) (((a) > (b)) ? (a) : (b))



typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t b8;
typedef uint32_t b32;


#define ARENA_HEADER_SIZE (sizeof(ARENA))
#define ARENA_ALIGN (2 * sizeof(void *))

#define ARENA_NUM_SCRATCH 2
#define ARENA_SCRATCH_RESERVE MB(64)
#define ARENA_SCRATCH_COMMIT KB(64)


typedef enum ARENA_FLAG {
    ARENA_FLAG_NONE = 0,
    ARENA_FLAG_GROWABLE = (1 << 0),
    ARENA_FLAG_DECOMMIT = (1 << 1),
} ARENA_FLAG;

typedef struct ARENA {
    // Chained linked list of arenas.
    struct ARENA* current;
    struct ARENA* prev;

    //unsigned char* buf;
    //size_t bufLen;
    //size_t prevOffset;
    //size_t currOffset;

    u64 reserveSize;
    u64 commitSize;

    u64 basePos;
    u64 pos;
    u64 commitPos;

    u32 flags;
} ARENA;

typedef struct ARENA_TEMP {
    ARENA* arena;
    u64 startPos;
} ARENA_TEMP;


//extern int ArenaInitEx(ARENA* a, void* backingBuffer, size_t backingBufferLen);
extern ARENA* ArenaInit(u64 reserveSize, u64 commitSize, u32 flags);
extern void ArenaTerminate(ARENA* a);

//extern void* ArenaAllocAlign(ARENA* a, size_t size, size_t align);
// TODO: Implement.
//extern void* ArenaAlloc(ARENA* a, size_t size);

extern void* ArenaPush(ARENA* arena, size_t size, b32 nonZero);
// Zeroes out the memory automatically.
#define ArenaPushStruct(arena, T) (T *) ArenaPush(arena, sizeof(T), 0)
#define ArenaPushStructNZ(arena, T) (T *) ArenaPush(arena, sizeof(T), 1)
#define ArenaPushArrayZero(arena, T, N) (T *) ArenaPush(arena, sizeof(T) * (N), 0)
// Uninitialized memory for speed.
#define ArenaPushArrayNZ(arena, T, N) (T *) ArenaPush(arena, sizeof(T) * (N), 1)






extern void* ArenaResizeAlign(ARENA* a, void* oldMemory, size_t oldSize, size_t newSize, size_t align);

extern void ArenaFreeAll(ARENA* arena);
#define ArenaClear(arena) ArenaFreeAll(arena)

extern void ArenaPop(ARENA* arena, size_t size);
extern void ArenaPopTo(ARENA* arena, size_t pos);

extern u64 ArenaGetPos(ARENA* arena);


extern ARENA_TEMP ArenaTempBegin(ARENA* arena);
extern void ArenaTempEnd(ARENA_TEMP temp);

extern ARENA_TEMP ArenaScratchGet(ARENA** conflicts, u32 nConflicts);
extern void ArenaScratchRelease(ARENA_TEMP scratch);

#endif
