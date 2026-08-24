#ifndef IMAGE_PARSE_H_
#define IMAGE_PARSE_H_ 1

#include "../base/base_defs.h"
#include "../base/base_string.h"
#include "../misc/arena.h"

#include "bitstream.h"

/*
   Some basic image parsing.
*/


typedef struct PARSEIMAGE {
    b32 valid;
    unsigned char* data;
    size_t width;
    size_t height;
    int channels;
} PARSEIMAGE;

extern void ParseDeflate(BITSTREAM* bs, u8* out, u64 outSize);

extern PARSEIMAGE ParseImage(ARENA* arena, STRING8 fileBuf, int nChannels);

#endif /* IMAGE_PARSE_H_ */
