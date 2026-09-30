#include "lzss.h"

#include <stdlib.h>

static char lzss_dict[0x2001] = { 0 };
static LzssTreeNode lzss_tree[0x2001] = { 0 };

/*
   Bit writer.
*/

void BitWriter::writeBit(int bit)
{
    _bitBuffer = (_bitBuffer << 1) | (bit & 1);
    _bitCount++;
    if (_bitCount == 8)
    {
        // Advance to next element.
        _buffer.push_back(_bitBuffer);
        _bitBuffer = 0;
        _bitCount = 0;
    }
}

void BitWriter::writeBits(int value, int nBits)
{
    for (int i = nBits - 1; i >= 0; --i)
    {
        writeBit((value >> i) & 1);
    }
}

void BitWriter::flush(void)
{
    if (_bitCount > 0)
    {
        _bitBuffer <<= (8 - _bitCount);
        _buffer.push_back(_bitBuffer);
        _bitBuffer = 0;
        _bitCount = 0;
    }
}

size_t BitWriter::getBytesWritten() const
{
    return _buffer.size();
}

const char* BitWriter::getBuffer() const
{
    return _buffer.data();
}

/*
   Bit reader.
*/

void BitReader::shiftBitBuffer(void)
{
    _bitBuffer >>= 1;
}

char BitReader::getBitBuffer(void)
{
    return _bitBuffer;
}

int BitReader::readBit(void)
{
    if ((int) _bitBuffer == 0x80)
    {
        if ((int) (_ptr - _data) < _size)
        {
            _currentByte = *_ptr;
            _ptr++;
        }
        else
        {
            _currentByte = 0;
        }
    }
    int bit = (_currentByte & _bitBuffer) ? 1 : 0;

    _bitBuffer >>= 1;
    if (_bitBuffer == 0)
    {
        _bitBuffer = 0x80;
    }

    return bit;
}

// Reads nBits, building the value MSB to LSB in a BE way.
int BitReader::readBits(int nBits)
{
    int value = 0;
    for (int i = 0; i < nBits; ++i)
    {
        value = (value << 1) | readBit();
    }

    return value;
}


char* Lzss::decompress(char *in, int compressedSize, char* out, int decompressedSize)
{
    if (out == NULL)
    {
        out = (char *) malloc(decompressedSize);
        if (out == NULL)
        {
            return NULL;
        }
    }

    BitReader reader(in, compressedSize);
    char* writePtr = out;
    uint32_t ringBufferIndex = 1;
    char* outEnd = out + decompressedSize;

    while (1)
    {
        int controlBit = reader.readBit();

        if (controlBit == 1)
        {
            char literal = (char) reader.readBits(8);
            if (writePtr >= outEnd)
            {
                break;
            }

            *writePtr = literal;
            writePtr++;

            // Update dictionary.
            lzss_dict[ringBufferIndex] = literal;
            ringBufferIndex = (ringBufferIndex + 1) & 0x1fff;
        }
        else
        {
            int matchOffset = reader.readBits(13);

            if (matchOffset == 0)
            {
                break;
            }

            int lengthBits = reader.readBits(4);
            int copyCount = lengthBits + 3;

            for (int i = 0; i < copyCount; ++i)
            {
                if (writePtr >= outEnd)
                {
                    break;
                }

                int srcIndex = (matchOffset + i) & 0x1fff;
                char val = lzss_dict[srcIndex];

                *writePtr = val;
                writePtr++;

                lzss_dict[ringBufferIndex] = val;
                ringBufferIndex = (ringBufferIndex + 1) & 0x1fff;
            }
        }
    }

    return out;
}

#define LZSS_OFFSET_BITS 13
#define LZSS_LENGTH_BITS 4
#define LZSS_DICTSIZE (1 << LZSS_OFFSET_BITS)
#define LZSS_LOOKAHEAD_SIZE ((1 << LZSS_LENGTH_BITS) + 2)
#define LZSS_DICTSIZE_MASK (LZSS_DICTSIZE - 1)
#define LZSS_DICTPOS_MOD(pos, amount) ((pos + amount) & LZSS_DICTSIZE_MASK)

char* Lzss::compress(char *in, int uncompressedSize, int *compressedSize)
{
    if (!in || uncompressedSize < 0)
    {
        return NULL;
    }

    BitWriter writer;
    int lookaheadBytes = 0;
    int dictValue = 0;
    int matchLength = 0;
    int matchOffset = 0;
    char* inCursor = in;
    uint32_t dictHead = 1;

    Lzss::initEncoderState();

    int i;
    for (i = 0; i < LZSS_LOOKAHEAD_SIZE; ++i)
    {
        if ((int) (inCursor - in) >= uncompressedSize)
        {
            dictValue = -1; // EOF.
        }
        else
        {
            dictValue = *inCursor++;
        }

        if (dictValue == -1)
        {
            break;
        }

        lzss_dict[dictHead + 1] = (char) dictValue;
    }
    lookaheadBytes = i;

    Lzss::initTree(dictHead);

    while (lookaheadBytes > 0)
    {
        if (matchLength > lookaheadBytes)
        {
            matchLength = lookaheadBytes;
        }

        if (matchLength <= 2)
        {
            writer.writeBit(1);

            char literalByte = lzss_dict[dictHead];
            writer.writeBits(literalByte, 8);

            matchLength = 1; // We consumed 1 byte.
        }
        else
        {
            writer.writeBit(0);
            writer.writeBits(matchOffset, LZSS_OFFSET_BITS);
            writer.writeBits(matchLength - 3, LZSS_LENGTH_BITS);
        }


        int bytesProduced = matchLength;
        for (i = 0; i < bytesProduced; ++i)
        {
            // Remove the old entry at current position.
            Lzss::deleteString(LZSS_DICTPOS_MOD(dictHead, LZSS_LOOKAHEAD_SIZE));

            // Read next byte from input.
            if ((int) (inCursor - in) >= uncompressedSize)
            {
                dictValue = -1;
            }
            else
            {
                dictValue = *inCursor++;
            }

            // Handle buffer refill/EOF.
            if (dictValue == -1)
            {
                lookaheadBytes--;
            }
            else
            {
                lzss_dict[LZSS_DICTPOS_MOD(dictHead, LZSS_LOOKAHEAD_SIZE)] = (char) dictValue;
            }


            // Advance ring buffer head.
            dictHead = LZSS_DICTPOS_MOD(dictHead, 1);

            // Find match for *next* iteration.
            if (lookaheadBytes != 0)
            {
                matchLength = Lzss::addString(dictHead, &matchOffset);
            }
        }
    }

    writer.writeBit(0); // Bit 0: match flag.
    writer.writeBits(0, LZSS_OFFSET_BITS);
    writer.flush();

    size_t finalSize = writer.getBytesWritten();
    *compressedSize = (int) finalSize;

    char* out = (char *) malloc(finalSize);
    if (out == NULL)
    {
        return NULL;
    }

    memcpy(out, writer.getBuffer(), finalSize);
    return out;
}

void Lzss::initTree(int root)
{
    lzss_tree[LZSS_DICTSIZE].right = root;
    lzss_tree[root].parent = LZSS_DICTSIZE;
    lzss_tree[root].right = 0;
    lzss_tree[root].left = 0;
}

void Lzss::initEncoderState(void)
{
    int i;
    for (i = 0; i < LZSS_DICTSIZE; ++i)
    {
        lzss_dict[i] = 0;
    }

    for (i = 0; i < LZSS_DICTSIZE; ++i)
    {
        lzss_tree[i].parent = 0;
        lzss_tree[i].left = 0;
        lzss_tree[i].right = 0;
    }
}

int Lzss::addString(int newNode, int *matchPosition)
{
    int i;
    int* child;
    int delta;

    if (newNode == 0)
    {
        return 0;
    }

    int testNode = lzss_tree[LZSS_DICTSIZE].right;
    int matchLength = 0;

    for (;;)
    {
        for (i = 0; i < LZSS_LOOKAHEAD_SIZE; ++i)
        {
            delta = lzss_dict[LZSS_DICTPOS_MOD(newNode, i)] - lzss_dict[LZSS_DICTPOS_MOD(testNode, i)];

            if (delta != 0)
            {
                break;
            }
        }

        if (i >= matchLength)
        {
            matchLength = i;
            *matchPosition = testNode;

            if (matchLength >= LZSS_LOOKAHEAD_SIZE)
            {
                Lzss::replaceNode(testNode, newNode);
                return matchLength;
            }
        }

        if (delta >= 0)
        {
            child = &lzss_tree[testNode].right;
        }
        else
        {
            child = &lzss_tree[testNode].left;
        }

        if (*child == 0)
        {
            *child = newNode;
            lzss_tree[newNode].parent = testNode;
            lzss_tree[newNode].right = 0;
            lzss_tree[newNode].left = 0;
            return matchLength;
        }

        testNode = *child;
    }
}

void Lzss::deleteString(int p)
{
    if (lzss_tree[p].parent == 0)
    {
        return;
    }

    if (lzss_tree[p].right == 0)
    {
        Lzss::contractNode(p, lzss_tree[p].left);
    }
    else if (lzss_tree[p].left == 0)
    {
        Lzss::contractNode(p, lzss_tree[p].right);
    }
    else
    {
        int replacement = Lzss::findNextNode(p);
        Lzss::deleteString(replacement);
        Lzss::replaceNode(p, replacement);
    }
}

void Lzss::contractNode(int oldNode, int newNode)
{
    lzss_tree[newNode].parent = lzss_tree[oldNode].parent;

    if (lzss_tree[lzss_tree[oldNode].parent].right == oldNode) {
        lzss_tree[lzss_tree[oldNode].parent].right = newNode;
    } else {
        lzss_tree[lzss_tree[oldNode].parent].left = newNode;
    }

    lzss_tree[oldNode].parent = 0;
}

void Lzss::replaceNode(int oldNode, int newNode)
{
    int parent = lzss_tree[oldNode].parent;

    if (lzss_tree[parent].left == oldNode) {
        lzss_tree[parent].left = newNode;
    } else {
        lzss_tree[parent].right = newNode;
    }

    lzss_tree[newNode] = lzss_tree[oldNode];
    lzss_tree[lzss_tree[newNode].left].parent = newNode;
    lzss_tree[lzss_tree[newNode].right].parent = newNode;
    lzss_tree[oldNode].parent = 0;
}


int Lzss::findNextNode(int node)
{
    int next = lzss_tree[node].left;
    while (lzss_tree[next].right != 0)
    {
        next = lzss_tree[next].right;
    }
    return next;
}
