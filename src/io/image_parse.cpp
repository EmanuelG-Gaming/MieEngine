#include "parse.h"
#include "bitstream.h"
#include <string.h>
#include <math.h>

static const STRING8 pngFileHeader = { 8, (u8 *)("\x89" "PNG" "\r\n" "\x1A" "\n") };
static const STRING8 qoiFileHeader = { 4, (u8 *)("qoif") };

typedef enum PNG_CHUNK {
    PNG_NONE = 0,
    PNG_IHDR,
    PNG_IDAT,
    PNG_PLTE,
    PNG_IEND,
} PNG_CHUNK;


typedef enum PNG_COL {
    PNG_GRAY = 0,
    PNG_COLOR = 2,
    PNG_INDEX = 3,
    PNG_GRAY_A = 4,
    PNG_COLOR_A = 6,
} PNG_COL;


typedef enum PNG_FILTER {
    PNG_FILTER_NONE = 0,
    PNG_SUB = 1,
    PNG_UP = 2,
    PNG_AVG = 3,
    PNG_PAETH = 4,
} PNG_FILTER;

static u8 bytesPerPixel[] = {
    1, 0, 3, 3, 2, 0, 4
};

typedef struct IDAT_NODE {
    u8* data;
    u32 size;
    struct IDAT_NODE* next;
} IDAT_NODE;


typedef struct PNG_STATE {
    u8* data;
    u64 pos;
    PNG_CHUNK chunk;
    u32 chunkSize;

    u32 bitDepth;
    PNG_COL colorType;

    IDAT_NODE* idatFirst;
    IDAT_NODE* idatLast;
    u64 idatTotalSize;

    ARENA_TEMP tempArena;

    PARSEIMAGE png;

    u8* out;
    u64 outSize;
    u64 outPos;
} PNG_STATE;


typedef struct ARRAY_U8 {
    size_t len;
    u8* data;
} ARRAY_U8;

static inline u8 PNG_GetByte(PNG_STATE* state)
{
    return state->data[state->pos++];
}

#define BYTE() (PNG_GetByte(state))
#define U32() (BYTE() << 24 | BYTE() << 16 | BYTE() << 8 | BYTE())

#define PNG_CHUNK_ID(a, b, c, d) ((u32)(a) << 24 | (u32)(b) << 16 | (u32)(c) << 8 | (u32)(d))

static void ParsePngIhdr(ARENA* arena, PNG_STATE* state)
{
    state->png.width = U32();
    state->png.height = U32();

    state->bitDepth = BYTE();
    state->colorType = (PNG_COL) BYTE();

    u32 compressionMethod = BYTE();
    u32 filterMethod = BYTE();
    u32 interlaceMethod = BYTE();

    if (compressionMethod != 0)
    {
        fprintf(stderr, "%s: Invalid compression method %d!\n", __func__, compressionMethod);
        state->png.valid = FALSE;
        return;
    }

    if (filterMethod != 0)
    {
        fprintf(stderr, "%s: Invalid filter method %d!\n", __func__, filterMethod);
        state->png.valid = FALSE;
        return;
    }

    if (interlaceMethod != 0)
    {
        fprintf(stderr, "%s: Invalid interlace method %d!\n", __func__, interlaceMethod);
        state->png.valid = FALSE;
        return;
    }

    state->out = (u8 *) ArenaPush(arena, sizeof(u8) * state->png.width * state->png.height, 0);
    state->tempArena = ArenaTempBegin(arena);
}

static ARRAY_U8 PngDecompress(ARENA* arena, PNG_STATE* state)
{
    ARRAY_U8 out = CLITERAL(ARRAY_U8) {
        .len = state->png.width * state->png.height * bytesPerPixel[state->colorType] + state->png.height,
    };
    out.data = (u8 *) ArenaPush(arena, out.len, 1);
    _MEMSET(out.data, 0, out.len);

    BITSTREAM bs = CLITERAL(BITSTREAM) {
        .data = (u8 *) ArenaPush(arena, state->idatTotalSize, 1),
        .bitPos = 16,
        .nBytes = state->idatTotalSize - 1,
    };

    u64 pos = 0;

    for (IDAT_NODE* node = state->idatFirst; node != NULL; node = node->next)
    {
        _MEMCPY(bs.data + pos, node->data, node->size);
        pos += node->size;
    }

    ParseDeflate(&bs, out.data, out.len);

    return out;
}

static int PaethPredictor(int a, int b, int c)
{
    int p = a + b - c;
    int pa = abs(p-a);
    int pb = abs(p-b);
    int pc = abs(p-c);

    if (pa <= pb && pa <= pc) return a;
    if (pb <= pc) return b;

    return c;
}


#define DF_CORE_SWITCH(forHeader, A, B, C, index) \
    switch (filterType) { \
        case PNG_FILTER_NONE: \
        { \
            forHeader { \
                state->out[state->outPos] = data.data[index]; \
                state->outPos ++; \
            } \
        } break; \
        case PNG_SUB: \
        { \
            forHeader { \
                state->out[state->outPos] = data.data[index] + A; \
                state->outPos ++; \
            } \
        } break; \
        case PNG_UP: \
        { \
            forHeader { \
                state->out[state->outPos] = data.data[index] + B; \
                state->outPos ++; \
            } \
        } break; \
        case PNG_AVG: \
        { \
            forHeader { \
                state->out[state->outPos] = data.data[index] + ((A+B) >> 1); \
                state->outPos ++; \
            } \
        } break; \
        case PNG_PAETH: \
        { \
            forHeader { \
                state->out[state->outPos] = data.data[index] + PaethPredictor(A, B, C); \
                state->outPos ++; \
            } \
        } break; \
        default: \
            fprintf(stderr, "%s: Invalid filter type: %u (expected 0-4)\n", __func__, filterType); \
            return; \
            break; \
    } \


#define PIXEL_BYTES() bytesPerPixel[state->colorType]


static void PngDefilter(PNG_STATE* state, ARRAY_U8 data, int nChannels)
{
    PNG_FILTER filterType = (PNG_FILTER) data.data[0];
    u64 byteWidth = 1 + state->png.width * bytesPerPixel[state->colorType];
    u64 realByteWidth = state->png.width * nChannels;

    if (bytesPerPixel[state->colorType] >= 3 && bytesPerPixel[state->colorType] == nChannels)
    {
        DF_CORE_SWITCH(for (u32 j = 1; j < PIXEL_BYTES() + 1; ++j), 0, 0, 0, j);

        DF_CORE_SWITCH(for (u32 j = PIXEL_BYTES() + 1; j < byteWidth; ++j), state->out[state->outPos - PIXEL_BYTES()], 0, 0, j);

        for (int i = 01; i < state->png.height; ++i)
        {
            filterType = (PNG_FILTER) data.data[i * byteWidth];

            DF_CORE_SWITCH(for (u32 j = 1; j < PIXEL_BYTES() + 1; ++j), 0, state->out[state->outPos - byteWidth + 1], 0, j + i * byteWidth);

            DF_CORE_SWITCH(for (u32 j = PIXEL_BYTES() + 1; j < byteWidth; ++j),
                state->out[state->outPos - PIXEL_BYTES()],
                state->out[state->outPos - byteWidth + 1],
                state->out[state->outPos - byteWidth + 1 - PIXEL_BYTES()],
                j + i * byteWidth);
        }
    }
    else if (bytesPerPixel[state->colorType] == 3 && nChannels == 4)
    {
        // Opaque image. Set alpha channel to 255.
        DF_CORE_SWITCH(for (u32 j = 1; j < PIXEL_BYTES() + 1; ++j), 0, 0, 0, j);
        state->out[state->outPos++] = 255;

        u64 thirdWidth = (byteWidth - 1) / 3;

        DF_CORE_SWITCH(
            for (u32 k = 1; k < thirdWidth; ++k, state->out[state->outPos++] = 255)
                for (u32 j = k*3 + 1; j < k*3 + 1 + PIXEL_BYTES(); ++j),
            state->out[state->outPos - nChannels],
            0, 0, j
        );

        for (u32 i = 1; i < state->png.height; ++i)
        {
            filterType = (PNG_FILTER) data.data[i * byteWidth];

            DF_CORE_SWITCH(for (u32 j = 1; j < PIXEL_BYTES() + 1; ++j),
                0, state->out[state->outPos - realByteWidth], 0, j + i*byteWidth);
            state->out[state->outPos] = 255;

            DF_CORE_SWITCH(
                for (u32 k = 1; k < thirdWidth; ++k, state->out[state->outPos++] = 255)
                    for (u32 j = k*3 + 1; j < k*3 + 1 + PIXEL_BYTES(); ++j),
                state->out[state->outPos - nChannels],
                state->out[state->outPos = realByteWidth],
                state->out[state->outPos - realByteWidth - nChannels],
                j + i*byteWidth);
        }
    }
    else
    {
        fprintf(stderr, "%s: Unsupported color type!\n", __func__);
    }
}

static void ParsePngChunk(ARENA* arena, PNG_STATE* state, int nChannels)
{
    state->chunkSize = U32();
    u32 chunkId = U32();

    switch (chunkId)
    {
        case PNG_CHUNK_ID('I', 'H', 'D', 'R'):
        {
            state->chunk = PNG_IHDR;

            ParsePngIhdr(arena, state);
            state->pos += 4;
        } break;

        case PNG_CHUNK_ID('I', 'D', 'A', 'T'):
        {
            state->chunk = PNG_IDAT;

            IDAT_NODE* node = (IDAT_NODE *) ArenaPush(arena, sizeof(IDAT_NODE), 1);
            *node = CLITERAL(IDAT_NODE) {
                .data = state->data + state->pos,
                .size = state->chunkSize,
            };
            SLL_APPEND_BACK(state->idatFirst, state->idatLast, node);

            state->idatTotalSize += state->chunkSize;
            state->pos += state->chunkSize + 4;
        } break;

        case PNG_CHUNK_ID('P', 'L', 'T', 'E'):
        {
            fprintf(stderr, "%s: TODO: PLTE. Skipping over...\n", __func__);

            state->chunk = PNG_PLTE;
            state->pos += state->chunkSize + 4;
        } break;

        case PNG_CHUNK_ID('I', 'E', 'N', 'D'):
        {
            state->chunk = PNG_IEND;

            ARRAY_U8 pngData = PngDecompress(arena, state);
            PngDefilter(state, pngData, nChannels);

            state->pos += state->chunkSize + 4;
            break;
        } break;

        default:
        {
            fprintf(stderr, "%s: Unhandled PNG Chunk: %.*s\n", __func__, 4, (const char *) state->data + state->pos - 4);
            state->pos += state->chunkSize + 4;
        } break;
    }
}

static PARSEIMAGE ParsePng(ARENA* arena, STRING8 fileBuf, u32 nChannels)
{
    if (Str8Equal(pngFileHeader, Str8Substr(fileBuf, 0, 8)))
    {
        fprintf(stderr, "%s: Invalid PNG header. Not a PNG file.\n", __func__);
        return CLITERAL(PARSEIMAGE) { .valid = FALSE };
    }


    PNG_STATE state;
    state.data = fileBuf.data + 8;
    state.chunk = PNG_NONE;
    state.png = CLITERAL(PARSEIMAGE) { .valid = TRUE };

    while (state.chunk != PNG_IEND)
    {
        ParsePngChunk(arena, &state, nChannels);
    }

    if (state.tempArena.startPos)
    {
        ArenaTempEnd(state.tempArena);
    }

    state.png.channels = nChannels;
    state.png.data = state.out;

    return state.png;
}

#undef PBYTE
#undef BYTE
#undef U32


#define PBYTE() (data[pos])
#define BYTE() (data[pos++])
#define U32() \
    (data[pos] << 24 | data[pos+1] << 16 | data[pos+2] << 8 | data[pos+3]); \
    pos += 4;

#define PIXEL_INDEX(p) ((p.r * 3 + p.g * 5 + p.b * 7 + p.a * 11) & 63)
#define WRITE_PIXEL(p) do { \
    out.data[outPos + 0] = p.r; \
    out.data[outPos + 1] = p.g; \
    out.data[outPos + 2] = p.b; \
    out.data[outPos + 3] = p.a; \
} while (0)


// QOI pixel.
typedef struct QPIXEL {
    u8 r, g, b, a;
} QPIXEL;

static PARSEIMAGE ParseQoi(ARENA* arena, STRING8 fileBuf, u32 nChannels)
{
    if (fileBuf.len < 14 || !Str8Equal(qoiFileHeader, Str8Substr(fileBuf, 0, 4)))
    {
        fprintf(stderr, "%s: Invalid QOI header. Not a QOI file.\n", __func__);
        return CLITERAL(PARSEIMAGE) { .valid = FALSE };
    }

    PARSEIMAGE out;
    out.valid = TRUE;

    u8* data = fileBuf.data;
    u64 pos = 4;

    out.width = U32();
    out.height = U32();
    BYTE();

    out.channels = nChannels;
    u32 colorspace = BYTE();
    UNUSED(colorspace);

    u64 outPos = 0;
    u64 outSize = out.width * out.height * out.channels;
    // + 1 to prevent overflow in WRITE_PIXEL.
    out.data = (u8 *) ArenaPush(arena, sizeof(u8) * (outSize + 1), 0);

    QPIXEL arr[64] = { 0 };
    QPIXEL pixel;
    pixel.a = 255;

    while (outPos < outSize)
    {
        switch (PBYTE())
        {
            case 0b11111110:
            {
                pos++;
                pixel.r = BYTE();
                pixel.g = BYTE();
                pixel.b = BYTE();

                arr[PIXEL_INDEX(pixel)] = pixel;
                WRITE_PIXEL(pixel);
            } break;

            case 0b11111111:
            {
                pos++;
                pixel.r = BYTE();
                pixel.g = BYTE();
                pixel.b = BYTE();
                pixel.a = BYTE();

                arr[PIXEL_INDEX(pixel)] = pixel;
                WRITE_PIXEL(pixel);
            } break;

            default:
            {
                switch ((PBYTE() & 0b11000000) >> 6)
                {
                    case 0b00:
                    {
                        u8 index = BYTE() & 0b00111111;
                        pixel = arr[index];
                        WRITE_PIXEL(pixel);
                    } break;

                    case 0b01:
                    {
                        u8 dr = ((PBYTE() & 0b00110000) >> 4) - 2;
                        u8 dg = ((PBYTE() & 0b00001100) >> 2) - 2;
                        u8 db = ((PBYTE() & 0b00000011) >> 0) - 2;

                        pixel.r += dr;
                        pixel.g += dg;
                        pixel.b += db;

                        arr[PIXEL_INDEX(pixel)] = pixel;
                        WRITE_PIXEL(pixel);
                    } break;

                    case 0b10:
                    {
                        u8 diffG = ((BYTE()) & 0b00111111) - 32;
                        u8 diffR = (((PBYTE() & 0b11110000) >> 4) - 8) + diffG;
                        u8 diffB = ((BYTE() & 0b00001111) - 8) + diffG;

                        pixel.r += diffR;
                        pixel.g += diffG;
                        pixel.b += diffB;

                        arr[PIXEL_INDEX(pixel)] = pixel;
                        WRITE_PIXEL(pixel);
                    } break;

                    case 0b11:
                    {
                        u8 length = BYTE() & 0b00111111;

                        do
                        {
                            // RLE.
                            WRITE_PIXEL(pixel);
                        } while (length--);
                    } break;
                }
            } break;
        }
    }

    ArenaPop(arena, 4 - nChannels);
    return out;
}


extern PARSEIMAGE ParseImage(ARENA* arena, STRING8 fileBuf, u32 nChannels)
{
    if (Str8Equal(pngFileHeader, Str8Substr(fileBuf, 0, pngFileHeader.len))) {
        return ParsePng(arena, fileBuf, nChannels);
    } else if (Str8Equal(qoiFileHeader, Str8Substr(fileBuf, 0, qoiFileHeader.len))) {
        return ParseQoi(arena, fileBuf, nChannels);
    }

    fprintf(stderr, "%s: Unsupported image format.\nEither use PNG or QOI.\n", __func__);
    return CLITERAL(PARSEIMAGE) { 0 };
}

