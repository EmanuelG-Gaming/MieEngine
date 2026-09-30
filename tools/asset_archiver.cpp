#include <stdio.h>
#include "../ext/file_lib.h"
#include "../ext/file_lib.cpp"

#include "../ext/lzss.h"
#include "../ext/lzss.cpp"


char archive[2048];
int32_t fileCompressedSizes[512] = { 0 };



typedef struct AssetArchiveHeader {
    char magic[4];
    int32_t fileCount;
    int32_t compressedOffset;
    int32_t uncompressedOffset;
} AssetArchiveHeader;
#define ARCHIVE_HEADER_SIZE 16

typedef struct AssetFileData {
    int32_t offset;
    int32_t compressedSize;
    int32_t uncompressedSize;
    char name[64];
} AssetFileData;
#define ARCHIVE_FILE_METADATA_SIZE (12 + 64)



typedef struct ArchiveContext {
    FILE* fp;
    int32_t nFiles;
    uint32_t offset;
} ArchiveContext;




int WriteToFile(FILE* handle, void* data, size_t dataLen)
{
    fwrite(data, 1, dataLen, handle);
    return 0;
}



struct FileCategory {
    const char* filter;
    const char* dir;
};

static constexpr FileCategory fileCategories[] = {
    { "png;jpg;jpeg", "images/" },      // Images.
    { "ttf", "fonts/" },                // Fonts.
    { "bin;bytc", "bin/" },             // Binaries/bytecode.
    //{ "wav;ogg", "music/" },            // Music.
    //{ "wav;ogg", "sounds/" },           // Sounds.
};







int RetrieveFiles(ArchiveContext* ctx, FileInfoList* fileList,
    const char* dirPath, const char* filter)
{
    int num = CreateFileList(dirPath, fileList, 1, 1, 0, 0, filter);

    fprintf(stderr, "PASS:\n");

    // Count the file sizes first.
    FileInfo* finfo;
    for (int i = 0; i < num; ++i)
    {
        finfo = &fileList->list[i];
        fprintf(stderr, "%s\n", finfo->fileName);
    }

    return num;
}


int DoMainCompressed(const char* dirPath, const char* outPath)
{
    // Only archive files if their time has changed.

    ArchiveContext ctx;
    // Init context.
    ctx.fp = fopen(outPath, "wb");
    if (ctx.fp == NULL)
    {
        fprintf(stderr, "Couldn't archive file to path: %s\n", outPath);
        return -1;
    }
    fprintf(stderr, "WRITING TO FILE! %s\n", outPath);

    ctx.nFiles = 0;
    ctx.offset = ARCHIVE_HEADER_SIZE;

    // Count up the amount of files.

    FileInfoList fileLists[32];
    int listCount = 0;

    for (auto& fileCategory : fileCategories) {
        FileInfoList* lst = &fileLists[listCount];
        ctx.nFiles += RetrieveFiles(&ctx, lst, dirPath, fileCategory.filter);
        listCount++;
    }

    int32_t offsetFromHdr = 0;
    int32_t compressedOffsetFromHdr = 0;
    for (int j = 0; j < listCount; ++j) {
        FileInfoList* fileList = &fileLists[j];
        FileInfo* finfo;
        for (int i = 0; i < fileList->num; ++i)
        {
            finfo = &fileList->list[i];

            // Add the sizes.
            offsetFromHdr += finfo->size;
        }
    }

    // And then overwrite the buffer with some useful data.
    {
        char* buf = new char[ARCHIVE_HEADER_SIZE];
        buf[0] = 'A';
        buf[1] = 'R';
        buf[2] = 'C';
        buf[3] = '0';

        int32_t files = ctx.nFiles;
        memcpy(buf + 4, &files, sizeof(int32_t));

        // Compressed offset.
        int32_t magicOffset = 0xDEADBEEF;
        memcpy(buf + 8, &magicOffset, sizeof(int32_t));

        // Uncompressed offset.
        magicOffset = offsetFromHdr;
        memcpy(buf + 12, &magicOffset, sizeof(int32_t));

        WriteToFile(ctx.fp, buf, ARCHIVE_HEADER_SIZE);
        delete[] buf;
    }


    // Then put all the files there.
    int fileIdx = 0;
    //offsetFromHdr = 0;
    for (int j = 0; j < listCount; ++j) {
        FileInfoList* fileList = &fileLists[j];
        FileInfo* finfo;

        for (int i = 0; i < fileList->num; ++i)
        {
            char dirPath[256] = { 0 };
            finfo = &fileList->list[i];

            strcat(dirPath, finfo->absDirectoryPath);
            strcat(dirPath, finfo->fileName);

            fprintf(stderr, "dirpath: %s\n", dirPath);


            void* dataBuf = NULL;
            int size;
            int compressedSize;
            LoadFileMemRef(dirPath, &dataBuf, &size);

            char* outCompressed = Lzss::compress(reinterpret_cast<char *> (dataBuf), finfo->size, &compressedSize);
            free(dataBuf);

            fprintf(stderr, "comp size: %d uncomp size: %d\n", compressedSize, finfo->size);

            WriteToFile(ctx.fp, outCompressed, compressedSize); // NOT size.

            fileCompressedSizes[fileIdx] = compressedSize;
            compressedOffsetFromHdr += compressedSize;
            fileIdx++;
        }
    }

    // Backtrack and then change the header.
    {
        char compressedOffsetBytes[4];
        // Go back to beginnning of file to fill up the data.
        fseek(ctx.fp, 0 + 8, SEEK_SET);
        fread(compressedOffsetBytes, 1, 4, ctx.fp);

        int32_t value = compressedOffsetFromHdr+ARCHIVE_HEADER_SIZE;
        memcpy(compressedOffsetBytes, &value, sizeof(int32_t));

        WriteToFile(ctx.fp, compressedOffsetBytes, 4);

        // And then go to the uppermost file.
        fseek(ctx.fp, compressedOffsetFromHdr + ARCHIVE_HEADER_SIZE, SEEK_SET);
    }


    // And now for the array list.
    int32_t offs = ARCHIVE_HEADER_SIZE;
    for (int j = 0; j < listCount; ++j) {
        FileInfoList* fileList = &fileLists[j];
        FileInfo* finfo;

        for (int i = 0; i < fileList->num; ++i)
        {
            char dirPath[256] = { 0 };
            finfo = &fileList->list[i];

            strcat(dirPath, finfo->relDirectoryPath);
            strcat(dirPath, finfo->fileName);

            // TODO: Issue here.
            char* buf = new char[ARCHIVE_FILE_METADATA_SIZE];
            // offset 0
            int32_t offset = offs;
            memcpy(buf, &offset, sizeof(int32_t));
            // offset 4
            int32_t compressedSize = fileCompressedSizes[i];
            memcpy(buf + 4, &compressedSize, sizeof(int32_t));
            // offset 8
            int32_t uncSize = finfo->size;
            memcpy(buf + 8, &uncSize, sizeof(int32_t));

            // offset 12
            char* st = buf + 12;
            memset(st, 0, 64);
            memcpy(st, dirPath, strlen(dirPath));

            /*
            st[0] = 'g';
            st[1] = 'r';
            st[2] = 'e';
            st[3] = 'g';
            st[4] = 't';
            st[5] = 'e';
            st[6] = 'c';
            st[7] = 'h';
            */

            WriteToFile(ctx.fp, buf, ARCHIVE_FILE_METADATA_SIZE);
            delete[] buf;

            offs += compressedSize;
        }
    }


    // Release the file lists at the end.
    for (int i = 0; i < listCount; ++i)
    {
        ReleaseFileList(&fileLists[i]);
    }

    fclose(ctx.fp);


    fprintf(stderr, "WROTE TO FILE! %s\n", outPath);
    //fprintf(stderr, "bruh!!\n", __func__);
    fprintf(stderr, "Uncompressed offset: %d\n", offsetFromHdr);
    fprintf(stderr, "Compressed offset: %d\n", compressedOffsetFromHdr);
    fprintf(stderr, "Compression ratio: %f\n", offsetFromHdr / (float)  compressedOffsetFromHdr);

    return 0;
}

int DoMainUncompressed(const char* dirPath, const char* outPath)
{
    // Only archive files if their time has changed.

    ArchiveContext ctx;
    // Init context.
    ctx.fp = fopen(outPath, "wb");
    if (ctx.fp == NULL)
    {
        fprintf(stderr, "Couldn't archive file to path: %s\n", outPath);
        return -1;
    }
    fprintf(stderr, "WRITING TO FILE! %s\n", outPath);

    ctx.nFiles = 0;
    ctx.offset = ARCHIVE_HEADER_SIZE;

    FileInfoList fileLists[32];
    int listCount = 0;

    for (auto& fileCategory : fileCategories) {
        FileInfoList* lst = &fileLists[listCount];
        ctx.nFiles += RetrieveFiles(&ctx, lst, dirPath, fileCategory.filter);
        listCount++;
    }

    int32_t offsetFromHdr = 0;
    //int32_t compressedOffsetFromHdr = 0;
    for (int j = 0; j < listCount; ++j) {
        FileInfoList* fileList = &fileLists[j];
        FileInfo* finfo;
        for (int i = 0; i < fileList->num; ++i)
        {
            finfo = &fileList->list[i];

            offsetFromHdr += finfo->size;
        }
    }

    // And then overwrite the buffer with some useful data.
    {
        char* buf = new char[ARCHIVE_HEADER_SIZE];
        buf[0] = 'A';
        buf[1] = 'R';
        buf[2] = 'C';
        buf[3] = '0';

        int32_t files = ctx.nFiles;
        memcpy(buf + 4, &files, sizeof(int32_t));

        // Compressed offset.
        int32_t magicOffset = 0xFACEF00D;
        memcpy(buf + 8, &magicOffset, sizeof(int32_t));

        // Uncompressed offset.
        magicOffset = offsetFromHdr + ARCHIVE_HEADER_SIZE;
        memcpy(buf + 12, &magicOffset, sizeof(int32_t));

        WriteToFile(ctx.fp, buf, ARCHIVE_HEADER_SIZE);
        delete[] buf;
    }


    // Then put all the files there.
    int fileIdx = 0;
    //offsetFromHdr = 0;
    for (int j = 0; j < listCount; ++j) {
        FileInfoList* fileList = &fileLists[j];
        FileInfo* finfo;

        for (int i = 0; i < fileList->num; ++i)
        {
            char dirPath[256] = { 0 };
            finfo = &fileList->list[i];

            strcat(dirPath, finfo->absDirectoryPath);
            strcat(dirPath, finfo->fileName);

            fprintf(stderr, "dirpath: %s\n", dirPath);

            void* dataBuf = NULL;
            int size;
            LoadFileMemRef(dirPath, &dataBuf, &size);

            WriteToFile(ctx.fp, dataBuf, finfo->size);

            free(dataBuf);

            fileIdx++;
        }
    }

    // And now for the array list.
    int32_t offs = ARCHIVE_HEADER_SIZE;
    for (int j = 0; j < listCount; ++j) {
        FileInfoList* fileList = &fileLists[j];
        FileInfo* finfo;

        for (int i = 0; i < fileList->num; ++i)
        {
            char dirPath[256] = { 0 };
            finfo = &fileList->list[i];

            strcat(dirPath, finfo->relDirectoryPath);
            strcat(dirPath, finfo->fileName);

            // TODO: Issue here.
            char* buf = new char[ARCHIVE_FILE_METADATA_SIZE];
            // offset 0
            int32_t offset = offs;
            memcpy(buf, &offset, sizeof(int32_t));
            // offset 4
            int32_t compressedSize = 0xDEADA505;
            memcpy(buf + 4, &compressedSize, sizeof(int32_t));
            // offset 8
            int32_t uncSize = finfo->size;
            memcpy(buf + 8, &uncSize, sizeof(int32_t));

            // offset 12
            char* st = buf + 12;
            memset(st, 0, 64);
            memcpy(st, dirPath, strlen(dirPath));

            WriteToFile(ctx.fp, buf, ARCHIVE_FILE_METADATA_SIZE);
            delete[] buf;

            offs += finfo->size;
        }
    }


    // Release the file lists at the end.
    for (int i = 0; i < listCount; ++i)
    {
        ReleaseFileList(&fileLists[i]);
    }

    fclose(ctx.fp);


    fprintf(stderr, "WROTE TO FILE! %s\n", outPath);
    fprintf(stderr, "Uncompressed offset: %d\n", offsetFromHdr);

    return 0;
}


int main(int argc, const char* argv[])
{
    const char* inPath = "assets/";
    const char* outPath = "assets.bin";

    if (argc == 3)
    {
        inPath = argv[1];
        outPath = argv[2];
    }
    else if (argc == 1)
    {
        fprintf(stderr, "Running with default settings.\n");
    }
    else
    {
        fprintf(stderr, "USAGE: 'assetArchive directory/ output'\n");
        return -1;
    }

    DoMainUncompressed(inPath, outPath);

    return 0;
}
