#ifndef CCL_TYPES_H_
#define CCL_TYPES_H_

#include <stdint.h>

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

#define TRUE 1
#define FALSE 0

#define STATIC_ARR_LEN(arr) (sizeof(arr) / (sizeof(*arr)))

#ifdef __cplusplus
#define CLITERAL(T) T
#else
#define CLITERAL(T) (T)
#endif

#ifndef _MEMSET
#define _MEMSET(src, val, bytes) memset(src, val, bytes)
#endif /* _MEMSET */

#ifndef _MEMCPY
#define _MEMCPY(dst, src, bytes) memcpy(dst, src, bytes)
#endif /* _MEMCPY */


#ifndef _MALLOC
#define _MALLOC(bytes) malloc(bytes)
#endif /* _MALLOC */

#ifndef _REALLOC
#define _REALLOC(ptr, bytes) realloc(ptr, bytes)
#endif /* _REALLOC */

#ifndef _CALLOC
#define _CALLOC(n, ptr) calloc(n, ptr)
#endif /* _CALLOC */

#ifndef _FREE
#define _FREE(ptr) free(ptr)
#endif /* _FREE */


#endif /* CCL_TYPES_H_ */
