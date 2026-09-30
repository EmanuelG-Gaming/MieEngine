#ifndef BASE_FMT_H_
#define BASE_FMT_H_ 1

#include "base_string.h"
#include "../mem/arena.h"


/*
Custom string formatting.
-------------------------

Basic syntax: "{(type):(format type and format settings)}"

Example: "{u32:04x}"

Supported types:
- Integers: i8, i16, i32, i64, u8, u16, u32, u64
- Booleans: b8, b32
- Floats: f32, f64
- Pointers: Any type followed by *
- Strings: cstring (null-terminated char*), string8
- Vectors: v2i16, v2i32, v2f32, v2f64
*/


extern String8 Str8_formatv(ARENA* arena, String8 fmt, va_list args);
extern String8 Str8_format(ARENA* arena, String8 fmt, ...);

extern String8 Str8_formatv_cstr(ARENA* arena, const char* fmt, va_list args);
extern String8 Str8_format_cstr(ARENA* arena, const char* fmt, ...);

#endif /* BASE_FMT_H_ */
