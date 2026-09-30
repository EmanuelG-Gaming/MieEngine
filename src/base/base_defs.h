#ifndef BASE_DEFS_H_
#define BASE_DEFS_H_ 1

/*
   For platform-independent sizes.
*/
#include <stdint.h>


/*
   Operating system.
*/

// Windows
#if !defined(HAS_WINDOWS)
#if defined(_WIN32)
#define HAS_WINDOWS 1
#else
#define HAS_WINDOWS 0
#endif
#endif

// UNIX (it's more than just MacOS)
#if !defined(HAS_UNIX)
#if !defined(_WIN32) && ((defined(__unix__) || defined(__unix)) || \
                        (defined(__APPLE__) && defined(__MACH__)))
#define HAS_UNIX 1
#else
#define HAS_UNIX 0
#endif
#endif

// Linux
#if !defined(HAS_LINUX)
#if defined(__linux__)
#define HAS_LINUX 1
#else
#define HAS_LINUX 0
#endif
#endif

/*
   Compilers.
*/

#if defined(__clang__)
#define COMPILER_CLANG
#elif defined(__GNUC__) || defined(__GNUG__)
#define COMPILER_GCC
#elif defined(_MSC_VER)
#define COMPILER_MSVC
#else
#define COMPILER_UNKNOWN
#endif

#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#define THREAD_LOCAL __thread
#elif defined(COMPILER_MSVC)
#define THREAD_LOCAL __declspec(thread)
#elif (__STDC_VERSION__ >= 201112L)
#define THREAD_LOCAL _Thread_local
#else
#error "Invalid compiler/version for thread variable. Use Clang, GCC, MSVC, or use C11 or greater."
#endif


/*
   Architecture stuff.
*/
#if defined(__x86_64__) || defined(_M_X64)
#define __ARCH_X64__
#elif defined(__i386__) || defined(_M_IX86)
#define __ARCH_X86__
#endif



/*
   DLL interactions.
*/

#if defined(_WIN32)
#define SHR_EXPORT __declspec(dllexport)
#elif defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#define SHR_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define SHR_EXPORT
#endif

#if defined(_WIN32)
#define SHR_IMPORT __declspec(dllimport)
#else
#define SHR_IMPORT
#endif


/*
   Statics.
*/

#define STATIC_ASSERT(condition, msg) \
    typedef char static_assertion__##msg[(condition) ? 1 : -1];

#define STATIC_GETSIZE(T) \
    char (*__sh_boom_##T)[sizeof(T)] = 1;


#define STATIC_ARR_LEN(arr) \
    (sizeof(arr) / sizeof(arr[0]))



#define UNUSED(expr) (void) expr


/*
   Shorthand typedefs.
*/

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

typedef uint8_t b8;
typedef uint32_t b32;

// Win32-specific naming.
// NOTE: Doesn't work with windows.h, so.
/*
typedef char BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef uint64_t QWORD;
typedef unsigned long long ULONGLONG;
*/

// Static checks.
// IEEE-754 strings.
STATIC_ASSERT(sizeof(f32) == 4, f32_size);
STATIC_ASSERT(sizeof(f64) == 8, f64_size);

// TODO: Ok it doesn't really work on Windows.
/*
#ifndef DWORD_PTR
#ifdef __ARCH_X64__
#define DWORD_PTR ULONGLONG
#else
#define DWORD_PTR DWORD
#endif
#endif

#ifndef LONG_PTR
#ifdef __ARCH_X64__
#define LONG_PTR __int64
#else
#define LONG_PTR int
#endif
#endif
*/

#define TRUE 1
#define FALSE 0


/*
   Stdlib memory allocation functions.
*/

#ifndef _MALLOC
#define _MALLOC(bytes) malloc(bytes)
#endif /* _MALLOC */

#ifndef _CALLOC
#define _CALLOC(val, bytes) calloc(val, bytes)
#endif /* _CALLOC */

#ifndef _REALLOC
#define _REALLOC(ptr, bytes) realloc(ptr, bytes)
#endif /* _REALLOC */

#ifndef _FREE
#define _FREE(ptr) free(ptr)
#endif /* _FREE */

/*
   The four functions that GCC and Clang
   can emit calls to for vectorization.
*/

#ifndef _MEMCPY
#define _MEMCPY(src, dst, count) memcpy(src, dst, count)
#endif /* _MEMCPY */

#ifndef _MEMSET
#define _MEMSET(buf, val, bytes) memset(buf, val, bytes)
#endif /* _MEMSET */

#ifndef _MEMCMP
#define _MEMCMP(src, dst, count) memcmp(src, dst, count)
#endif /* _MEMCMP */

#ifndef _MEMMOVE
#define _MEMMOVE(src, dst, count) memmove(src, dst, count)
#endif /* _MEMMOVE */


#ifndef _inline
#define _inline inline
#endif

/*
   C++ keywords.
*/

#ifndef _CONSTEXPR
#define _CONSTEXPR constexpr
#endif


#ifdef __cplusplus
#define ALIGN_DECL(bytes) alignas(bytes)
#elif _MSC_VER
#define ALIGN_DECL(bytes) __declspec(align(bytes))
#else
#define ALIGN_DECL(bytes) __attribute((aligned(bytes)))
#endif

// On some C++ compilers like MSVC, this style of
// initializing things 'on the fly' (C99 feature) will not work.
// But Aggregate initialization is a C++11 feature, and MSVC historically
// didn't support C99 functionalities in C++.
#ifdef __cplusplus
#define CLITERAL(T) T
#else
#define CLITERAL(T) (T)
#endif


#ifdef __cplusplus
#define DECLTYPE_CAST(T) decltype(T)
#else
#define DECLTYPE_CAST(T)
#endif

// TODO: MSVC implementation along with GCC extension?
#ifdef __cplusplus
#define TYPEOF_CAST(T) typeof(T)
#else
#define TYPEOF_CAST(T) __typeof__(T)
#endif


#ifndef _ASSERT
#define _ASSERT(expr) assert(expr)
#endif


#define IDX(x, y, w) ((y) * (w) + (x))
#define SWAP(a, b) do { \
    TYPEOF_CAST(a) tmp = (a); \
    (a) = (b); \
    (b) = tmp; \
} while(0)

#define ARRAY_SIZE(arr) (size_t)((sizeof(arr) / sizeof((arr)[0])))

#define CONCAT_NX(a, b) a##b
#define CONCAT(a, b) CONCAT_NX(a, b)
#define MULTILINE_STR(...) #__VA_ARGS__




#define KB(n) (((uint64_t)(n)) << 10)
#define MB(n) (((uint64_t)(n)) << 20)
#define GB(n) (((uint64_t)(n)) << 30)
#define TB(n) (((uint64_t)(n)) << 40)


#define IS_POW_2(x) ((x & (x-1)) == 0)


#define ALIGN_UP_POW2(n, p) (((n) + ((p) - 1)) & (~((p) - 1)))


#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define CLAMP(v, m, M) (MAX(MIN((v), (M)), (m)))


#define CLAMP_TOP(a, max) MIN(a, max)
#define CLAMP_BOTTOM(a, min) MAX(a, min)

#define MIN3(a, b, c) (MIN(MIN(a, b), c))
#define MAX3(a, b, c) (MAX(MAX(a, b), c))


/*
   Linked list utilities.
   f = first.
   l = last.
   n = element.
*/

#define SLL_APPEND_FRONT(f, l, n) ((f) == NULL ? \
        (f) = (l) = (n) : \
        ((n)->next = (f), (f) = (n)))

#define SLL_APPEND_BACK(f, l, n) ((f) == NULL ? \
        ((f) = (l) = (n)) : \
        ((l)->next = (n), (l) = (n)), \
        ((n)->next = NULL))

#define SLL_POP_FRONT(f, l) ((f) == (l) ? \
        ((f) = (l) = NULL) : \
        ((f) = (f)->next)

#define SLL_STACK_PUSH(f, n) ((n)->next = (f), (f) = (n))
#define SLL_STACK_POP(f) ((f) == NULL ? NULL : ((f) = (f)->next))


#define DLL_APPEND_BACK(f, l, n) ((f) == NULL ? \
        ((f) = (l) = (n), (n)->next = (n)->prev = NULL) : \
        ((n)->prev = (l), (l)->next = (n), (l) = (n), (n)->next = NULL))

#define DLL_APPEND_FRONT(f, l, n) DLL_APPEND_BACK(l, f, n)

#define DLL_REMOVE(f, l, n) ( \
        (f) == (n) ? \
            ((f) == (l) ? \
                ((f) = (l) = (NULL)) : \
                ((f) = (f)->next, (f)->prev = NULL)) : \
            (l) == (n) ? \
                ((l) = (l)->prev, (l)->next = NULL) : \
                ((n)->next->prev = (n)->prev, \
                (n)->prev->next = (n)->next))


#endif // BASE_DEFS_H_
