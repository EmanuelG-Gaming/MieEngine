#ifndef BITSTREAM_H_
#define BITSTREAM_H_ 1

#include "../base/base_defs.h"

/*
   Essentially a bitmap/bit field/bitset.
*/

static const u32 BS_bitMasks[] = {
    0x0000,
    0x0001, 0x0003, 0x0007, 0x000f, 0x001f, 0x003f, 0x007f, 0x00ff,
    0x01ff, 0x03ff, 0x07ff, 0x0fff, 0x1fff, 0x3fff, 0x7fff, 0xffff,
};

typedef struct BITSTREAM {
    unsigned char* data;
    size_t bitPos;
    size_t nBytes;
} BITSTREAM;

static u32 BS_PeekBits(BITSTREAM* bs, u32 bits)
{
    u32 bytes = (bs->bitPos >> 3) & ~1;
    u32 out = *(u32 *) (bs->data + bytes);
    out >>= (bs->bitPos & 0xf);
    return out & BS_bitMasks[bits];
}
static u32 BS_GetBits(BITSTREAM* bs, u32 bits)
{
    u32 bytes = (bs->bitPos >> 3) & ~1;
    u32 out = *(u32 *) (bs->data + bytes);
    out >>= (bs->bitPos & 0xf);
    bs->bitPos += bits;
    return out & BS_bitMasks[bits];
}
static u8* BS_GetPtr(BITSTREAM* bs)
{
    return bs->data + ((bs->bitPos >> 3) & ~1);
}


#define BS_FlushByte(bs) ( \
    ((bs)->bitPos & 7) == 0 ? 0 : \
    0, (bs)->bitPos += 9 - ((bs)->bitPos & 7))

#define BS_I32(bs) ((u32) (BS_GetBits((bs), 16) | (BS_GetBits((bs), 16) << 16)))
#define BS_U32(bs) ((u32) (BS_GetBits((bs), 16) | (BS_GetBits((bs), 16) << 16)))

#endif /* BITSTREAM_H_ */
