#ifndef LZSS_H_
#define LZSS_H_ 1

#include <vector>

class BitReader {
public:
    BitReader(char* data, int size) : _data(data), _size(size), _ptr(data), _bitBuffer(0x80) {}

    int readBit(void);
    int readBits(int nBits);
    char getBitBuffer(void);
    void shiftBitBuffer(void);

private:
    char* _data;
    int _size;
    char* _ptr;
    char _currentByte;
    char _bitBuffer; // Bit position within the byte.
};

class BitWriter {
public:
    BitWriter() : _bitBuffer(0), _bitCount(0) {}
    void writeBit(int bit);
    void writeBits(int value, int nBits);
    void flush(void);
    size_t getBytesWritten() const;
    const char* getBuffer() const;

private:
    std::vector<char> _buffer;
    char _bitBuffer;
    int _bitCount;
};

/* Binary tree for searching the Lzss duplicates. */
struct LzssTreeNode {
    int parent;
    int left;
    int right;
};

class Lzss {
public:
    static char* compress(char* in, int uncompressedSize, int* compressedSize);
    static char* decompress(char* in, int compressedSize, char* out, int decompressedSize);

    static void initTree(int root);
    static void initEncoderState(void);
    static int addString(int newNode, int* matchPosition);
    static void deleteString(int p);
    static void contractNode(int oldNode, int newNode);
    static void replaceNode(int oldNode, int newNode);
    static int findNextNode(int node);
};



#endif /* LZSS_H_ */
