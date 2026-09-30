#ifndef FILE_LIB_H_
#define FILE_LIB_H_ 1

/*
   File and directory handling library.
   To be used for tools.

   References:
   https://github.com/Yumetodo/DxLib/tree/master/source/Tool/FileLib.h
*/

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS 1
#endif

#ifndef DO_INLINE_ASM
#define DO_INLINE_ASM __asm
#endif

#include <stdint.h>
#include <stdio.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef struct FileDate {
    u64 create;
    u64 lastAccess;
    u64 lastWrite;
} FileDate;

typedef struct FileInfo {
    char* fileName;
    char* relDirectoryPath;
    char* absDirectoryPath;
    u32 size;
    u32 isDirectory;
    FileDate date;
    u32 attributes;
} FileInfo;

typedef struct FileInfoList {
    FileInfo* list;
    int num;
} FileInfoList;

extern void EasyEncode(void* data, unsigned int size);
extern void EasyEncodeFileWrite(void* data, int size, FILE* fp);
extern void EasyEncodeFileRead(void* data, int size, FILE* fp);

extern int LoadFileMemRef(const char* path, void** dataBuf, int* size);
extern int LoadFileMem(const char* path, void* dataBuf, int* size);
extern int SaveFileMem(const char* path, void* data, int size);

extern int __CreateDirectory(const char* path);
extern int IsDirectory(const char* path);

extern int CreateFileInfo(const char* path, FileInfo* fileInfoBuffer);
extern int ReleaseFileInfo(FileInfo* fileInfo);
extern int SetFileTimestamp(const char* path, FileInfo* fileInfo);
extern int CompareFileTimestamp(FileInfo* finfo1, FileInfo* finfo2, bool create = true, bool lastAccess = true, bool lastWrite = true);


extern int CreateFileList(
    const char* directoryPath, FileInfoList* fileListInfo = NULL,
    int omitDirectory = 0, int subDirectory = 0,
    const char* omitName = NULL, const char* omitExName = NULL,
    const char* validExName = NULL);

extern int ReleaseFileList(FileInfoList* directoryInfo);

extern int AnalyseFilePath(
    const char* src, char* fullPath, char* dirPath,
    char* fileName, char* name, char* exeName,
    const char* currentDir = NULL);

extern int ConvertFullPath(const char* src, char* dest, const char* currentDir = NULL);

extern int AnalysisFileNameAndDirPath(const char* src, char* fileName = NULL, char* dirPath = NULL);
extern int AnalysisFileNameAndExeName(const char* src, char* name = NULL, char* exeName = NULL);

// Get changed exe name path.
extern int GetChangeExeNamePath(const char* src, char* dest, const char* exeName);

// Some string utilities.
extern void SetEnMark(char* pathBuf);
extern void SetChr(char* pathBuf, char ch);
extern void DelChr(char* pathBuf, char ch);

extern int GetExName(const char* path, char* exNameBuf);
extern int SetExName(const char* path, char* exName, char* destBuf);

extern int CheckTextData(void* buffer, int size);
extern int CheckTextFile(const char* path);

extern int CheckMultiByteChar(char* buf);

#endif /* FILE_LIB_H_ */
