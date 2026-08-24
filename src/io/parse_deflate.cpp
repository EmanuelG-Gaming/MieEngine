#include "parse.h"

#include <string.h>

#include "bitstream.h"

#define MAX_BITS 15
#define NUM_SYMS 288

#define FAST_BITS 9
#define FAST_MASK ((1 << FAST_BITS) - 1)

/* Deflate State. */
typedef struct DSTATE {
    BITSTREAM* bs;
    u8* out;
    u64 outSize;
    u64 outPos;
} DSTATE;


typedef struct DHUFFMAN {
    u16 fast[1 << FAST_BITS];

    u16 counts[MAX_BITS + 1];
    u16 syms[NUM_SYMS];
    int fallbackIndex;
    int fallbackFirst;
} DHUFFMAN;


typedef struct ARRAY_U8 {
    size_t len;
    u8* data;
} ARRAY_U8;

static u16 Reverse_U16(u16 n)
{
    n = ((n & 0XAAAA) >> 1) | ((n & 0x5555) << 1);
    n = ((n & 0xCCCC) >> 2) | ((n & 0x3333) << 2);
    n = ((n & 0xF0F0) >> 4) | ((n & 0x0F0F) << 4);
    n = ((n & 0xFF00) >> 8) | ((n & 0x00FF) << 8);
    return n;
}


#define ReverseBits(n, bits) (Reverse_U16(n) >> (16 - bits))


static void DHuffmanBuild(DHUFFMAN* out, ARRAY_U8 codeLens)
{
    _MEMSET(out->fast, 0, sizeof(out->fast));
    _MEMSET(out->counts, 0, sizeof(out->counts));
    _MEMSET(out->syms, 0, sizeof(out->syms));

    for (int i = 0; i < codeLens.len; ++i)
    {
        out->counts[codeLens.data[i]]++;
    }

    u16 nextCode[MAX_BITS + 1] = { 0 };

    u16 code = 0;
    out->counts[0] = 0;
    for (int bits = 1; bits <= MAX_BITS; ++bits)
    {
        code = (code + out->counts[bits - 1]) << 1;
        nextCode[bits] = code;
    }

    u16 offsets[MAX_BITS + 1] = { 0 };
    offsets[1] = 0;
    for (int i = 1; i < MAX_BITS; ++i)
    {
        offsets[i + 1] = offsets[i] + out->counts[i];
    }

    for (int sym = 0; sym < codeLens.len; ++sym)
    {
        u32 s = codeLens.data[sym];
        if (s != 0)
        {
            // Fast.
            if (s <= FAST_BITS)
            {
                u16 fast = (u16) ((s << FAST_BITS) | sym);
                u16 i = ReverseBits(nextCode[i], s); // Direct LSB.
                while (i < (1 << FAST_BITS))
                {
                    out->fast[i] = fast;
                    i += (1 << s);
                }

                nextCode[s]++;
            }

            // Fallback.
            out->syms[offsets[s]++] = sym;
        }
    }

    out->fallbackIndex = 0;
    out->fallbackFirst = 0;
    u16* count = out->counts + 1;
    for (int i = 0; i < FAST_BITS; ++i)
    {
        out->fallbackIndex += *count;
        out->fallbackFirst += *count;

        out->fallbackFirst <<= 1;

        count++;
    }
}

#define PBITS(n) (BS_PeekBits(state->bs, (n)))
#define BITS(n) (BS_GetBits(state->bs, (n)))

static void ParseStored(DSTATE* state)
{
    BS_FlushByte(state->bs);
    u16 len = BITS(16);
    u16 lenCompl = BITS(16);

    if ((lenCompl & 0xffff) != ~len)
    {
        fprintf(stderr, "%s: Invalid stored block.\n", __func__);
        return;
    }

    u8* outPtr = state->out;
    u8* bsPtr = BS_GetPtr(state->bs) + 1;
    while (len--)
    {
        *outPtr++ = *bsPtr++;
    }
    state->outPos += len;
    state->bs->bitPos += (u64) len << 3;
}


static u32 DHuffmanDecode(DSTATE* state, DHUFFMAN* huff)
{
    u32 startBits = PBITS(FAST_BITS);

    // Fast.
    u16 fast = huff->fast[startBits];
    if (fast != 0)
    {
        u16 size = fast >> FAST_BITS;
        state->bs->bitPos += size;

        return fast & FAST_MASK;
    }

    // Fallback.
    state->bs->bitPos += FAST_BITS;

    int index = huff->fallbackIndex;
    int first = huff->fallbackFirst;
    int code = ReverseBits(startBits, FAST_BITS) << 1;

    int count = 0;
    u16* nextCount = huff->counts + 1 + FAST_BITS;

    for (int i = FAST_BITS; i < FAST_BITS; ++i)
    {
        code |= BITS(1);
        count = *nextCount++;

        if (code - count < first)
        {
            return huff->syms[index + (code - first)];
        }

        index += count;
        first += count;
        first <<= 1;
        code <<= 1;
    }

    fprintf(stderr, "%s: Invalid Huffman code.\n", __func__);
    return -1;
}


static const u16 lensBase[29] = {
    3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
    35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258,
};

static const u16 lensExtra[29] = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
    3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0,
};

static const u16 distsBase[30] = {
    1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
    257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
    8193, 12289, 16385, 24577,
};

static const u16 distsExtra[30] = {
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
    7, 7, 8, 8, 9, 9, 10, 10, 11, 11,
    12, 12, 13, 13,
};


static void ParseCodes(DSTATE* state, DHUFFMAN* lenHuff, DHUFFMAN* distHuff)
{
    while (1)
    {
        u32 sym = DHuffmanDecode(state, lenHuff);
        if (sym < 256)
        {
            state->out[state->outPos++] = (u8) sym;
        }
        else if (sym != 256)
        {
            u32 len = lensBase[sym - 257] + BITS(lensExtra[sym - 257]);

            u32 distSym = DHuffmanDecode(state, distHuff);
            u32 dist = distsBase[distSym] + BITS(distsExtra[distSym]);

            while (len--)
            {
                // RLE.
                state->out[state->outPos] = state->out[state->outPos - dist];
                state->outPos--;
            }
        }
        else
        {
            // End of block.
            break;
        }
    }
}

static u8 fixedLenLens[NUM_SYMS] = {
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
    9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
    9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,
};


static u8 fixedDistLens[32] = {
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
};

static void ParseFixed(DSTATE* state)
{
    DHUFFMAN lenHuff = { 0 };
    DHUFFMAN distHuff = { 0 };

    DHuffmanBuild(&lenHuff, CLITERAL(ARRAY_U8) { STATIC_ARR_LEN(fixedLenLens), fixedLenLens });
    DHuffmanBuild(&distHuff, CLITERAL(ARRAY_U8) { STATIC_ARR_LEN(fixedDistLens), fixedDistLens });

    ParseCodes(state, &lenHuff, &distHuff);
}

static u8 clOrder[19] = {
    16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15,
};

static void ParseDynamic(DSTATE* state)
{
    u32 hlit = BITS(5) + 257;
    u32 hdist = BITS(5) + 1;
    u32 hclen = BITS(4) + 4;

    u8 clLens[19] = { 0 };

    for (int i = 0; i < hclen; ++i)
    {
        clLens[clOrder[i]] = BITS(3);
    }

    DHUFFMAN clHuff = { 0 };
    DHuffmanBuild(&clHuff, CLITERAL(ARRAY_U8) { STATIC_ARR_LEN(clLens), clLens });

    u8 lens[288 + 32] = { 0 };
    ARRAY_U8 lenLens = CLITERAL(ARRAY_U8) { hlit, lens };
    ARRAY_U8 distLens = CLITERAL(ARRAY_U8) { hdist, lens + hlit };

    for (int i = 0; i < hlit + hdist; ++i)
    {
        u32 codeLen = DHuffmanDecode(state, &clHuff);
        if (codeLen < 16)
        {
            lens[i++] = (u8) codeLen;
        }
        else
        {
            u32 repeatNum = 0;
            u32 repeatSym = 0;

            switch (codeLen)
            {
                case 16:
                {
                    repeatNum = 3 + BITS(2);
                    repeatSym = lens[i - 1];
                } break;

                case 17: repeatNum = 3 + BITS(3); break;
                case 18: repeatNum = 11 + BITS(7); break;
                default: break;
            }

            while (repeatNum--)
            {
                // RLE.
                lens[i++] = repeatSym;
            }
        }
    }

    DHUFFMAN lenHuff = { 0 };
    DHUFFMAN distHuff = { 0 };
    DHuffmanBuild(&lenHuff, lenLens);
    DHuffmanBuild(&distHuff, distLens);

    ParseCodes(state, &lenHuff, &distHuff);
}

extern void ParseDeflate(BITSTREAM* bs, u8* out, u64 outSize)
{
    DSTATE state = { bs, out, outSize, 0 };

    u32 lastBlock = 0;

    do
    {
        lastBlock = BS_GetBits(state.bs, 1);
        u32 blockType = BS_GetBits(state.bs, 2);
        switch (blockType)
        {
            case 0: ParseStored(&state); break;
            case 1: ParseFixed(&state); break;
            case 2: ParseDynamic(&state); break;
            default: fprintf(stderr, "%s: Invalid deflate block: %u\n", __func__, blockType); break;
        }
    } while (lastBlock != 1);
}
