#include <string.h>
#include <stdio.h>
#include <mbstring.h>

// TODO: Currently uses more OS-specific Windows stuff.
// Probably gonna make some Linux (POSIX) syscalls for compatiblity.
#include <windows.h>

#include "file_lib.h"

#define PATH_LENGTH 256
#define TEXTCHECKSIZE (0x2000)

static int EnumObject(char* path, char* currentPath, FileInfoList* fileList,
        int omitDirectory, int subDirectory, char** omitName, char** omitExName, char** validExName);

static int EnumObject(char* path, char* currentPath, FileInfoList* fileList,
        int omitDirectory, int subDirectory, char** omitName, char** omitExName, char** validExName)
{
    WIN32_FIND_DATA findData;
    HANDLE findHandle = INVALID_HANDLE_VALUE;
    int fileNum, isDirectory;
    char relDir[PATH_LENGTH];
    char* absDir;
    int relDirLength, absDirLen, startNum;

    absDir = path;
    absDirLen = strlen(absDir);
    strcpy(relDir, absDir + strlen(currentPath));
    relDirLength = strlen(relDir);

    {
        char temp[PATH_LENGTH];

        strcpy(temp, absDir);
        strcat(temp, "*");
        findHandle = FindFirstFile(temp, &findData);
        if (findHandle == INVALID_HANDLE_VALUE)
        {
            return -1;
        }
    }

    fileNum = 0;
    if (fileList != NULL)
    {
        startNum = fileList->num;
    }

    do
    {
        if (strcmp(findData.cFileName, ".") == 0 || strcmp(findData.cFileName, "..") == 0)
        {
            continue;
        }

        isDirectory = ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) ? 1 : 0;

        // Exclusion.
        if (validExName != NULL && isDirectory == 0)
        {
            int i;
            char* name;

            name = strchr(findData.cFileName, '.');
            if (name == NULL) name = const_cast<char *> ("");
            else name++;

            for (i = 0; validExName[i] != NULL && _stricmp(name, validExName[i]) != 0; ++i) {}
            if (validExName[i] == NULL) continue;
        }

        if (omitName != NULL)
        {
            int i;
            for (i = 0; omitName[i] != NULL && strcmp(findData.cFileName, omitName[i]) != 0; ++i) {}
            if (omitName[i] != NULL) continue;
        }


        if (omitExName != NULL && isDirectory == 0)
        {
            int i;
            char* name;

            name = strchr(findData.cFileName, '.');
            if (name == NULL) name = const_cast<char *>("");
            else name++;

            for (i = 0; omitExName[i] != NULL && strcmp(name, omitExName[i]) != 0; ++i) {}
            if (omitExName[i] != NULL) continue;
        }

        // Directory checking.
        if (isDirectory == 1 && subDirectory == 1)
        {
            int res;
            char tempAbs[PATH_LENGTH], tempRel[PATH_LENGTH];

            strcpy(tempAbs, absDir);
            strcat(tempAbs, findData.cFileName);
            strcat(tempAbs, "\\");

            strcpy(tempRel, relDir);
            strcpy(tempRel, findData.cFileName);
            strcat(tempRel, "\\");

            // Recursive.
            res = EnumObject(tempAbs, currentPath, fileList, omitDirectory, subDirectory, omitName, omitExName, validExName);
            if (res < 0)
            {
                goto err;
            }
            fileNum += res;
        }

        if (fileList != NULL && (isDirectory == 0 || (isDirectory == 1 && omitDirectory == 0)))
        {
            // Fill info with data.
            FileInfo* info;
            int fileNameLen;

            info = &fileList->list[fileList->num];
            memset(info, 0, sizeof(FileInfo));

            info->date.create = (((LONGLONG) findData.ftCreationTime.dwHighDateTime) << 32) + findData.ftCreationTime.dwLowDateTime;
            info->date.lastAccess = (((LONGLONG) findData.ftLastAccessTime.dwHighDateTime) << 32) + findData.ftLastAccessTime.dwLowDateTime;
            info->date.lastWrite = (((LONGLONG) findData.ftLastWriteTime.dwHighDateTime) << 32) + findData.ftLastWriteTime.dwLowDateTime;

            info->size = findData.nFileSizeLow;
            info->attributes = findData.dwFileAttributes;
            info->isDirectory = (u8) isDirectory;

            {
                fileNameLen = strlen(findData.cFileName);
                info->fileName = (char *) malloc((fileNameLen + 1) + (absDirLen + 1) + (relDirLength + 1));
                if (info->fileName == NULL)
                {
                    goto err;
                }
                info->relDirectoryPath = info->fileName + fileNameLen + 1;
                info->absDirectoryPath = info->relDirectoryPath + relDirLength + 1;

                strcpy(info->fileName, findData.cFileName);
                strcpy(info->relDirectoryPath, relDir);
                strcpy(info->absDirectoryPath, absDir);
            }

            fileList->num++;
        }

        fileNum++;
    } while (FindNextFile(findHandle, &findData) != 0);

    FindClose(findHandle);
    findHandle = INVALID_HANDLE_VALUE;

    return fileNum;

err:
    if (findHandle != INVALID_HANDLE_VALUE)
    {
        FindClose(findHandle);
    }

    // Free each individual file.
    if (fileList != NULL)
    {
        int i;

        for (i = startNum; i < fileList->num; ++i)
        {
            if (fileList->list[i].fileName != NULL)
            {
                free(fileList->list[i].fileName);
            }
        }
    }

    fprintf(stderr, "%s: error in enumeration of dir entries!\n", __func__);

    return -1;
}





// Symmetric encoding.
extern void EasyEncode(void* data, unsigned int size)
{
    if (size == 0)
    {
        return;
    }
#ifdef _WIN64
    unsigned int i;
    BYTE* p = (BYTE *) data;
    for (i = 0; i < size; ++i)
    {
        p[i] = (~p[i] << 4) | (~p[i] >> 4);
    }
#else
    unsigned long long tempSize, tempData;

    tempData = reinterpret_cast<unsigned long long>(data);
    tempSize = static_cast<unsigned long long>(size);

    DO_INLINE_ASM
    {
        mov edi, tempData
        mov ecx, tempSize;
LOOP1:
        mov al, [edi]
        not al
        rol al, 4
        inc edi

        dec ecx
        jnz LOOP1
    };
#endif
}

extern void EasyEncodeFileWrite(void* data, int size, FILE* fp)
{
    EasyEncode(data, size);
    fwrite(data, size, 1, fp);
    EasyEncode(data, size);
}

extern void EasyEncodeFileRead(void* data, int size, FILE* fp)
{
    fread(data, size, 1, fp);
    EasyEncode(data, size);
}

extern int LoadFileMemRef(const char* path, void** dataBuf, int* size)
{
    FILE* fp = NULL;
    void* buf = NULL;
    int sz;

    fp = fopen(path, "rb");
    if (fp == NULL) goto err;

    fseek(fp, 0, SEEK_END);
    sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    buf = malloc(sz);
    if (buf == NULL) goto err;

    fread(buf, sz, 1, fp);
    fclose(fp);

    if (dataBuf != NULL) *dataBuf = buf;
    if (size != NULL) *size = sz;

    return 0;
err:
    if (buf != NULL) free(buf);
    if (fp != NULL) fclose(fp);
    return -1;
}

extern int LoadFileMem(const char* path, void* dataBuf, int* size)
{
    FILE* fp = NULL;
    int sz;

    fp = fopen(path, "rb");
    if (fp == NULL) return -1;

    fseek(fp, 0, SEEK_END);
    sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (dataBuf)
    {
        fread(dataBuf, sz, 1, fp);
    }
    fclose(fp);

    if (size != NULL) *size = sz;

    return 0;
}

extern int SaveFileMem(const char* path, void* data, int size)
{
    FILE* fp;
    fp = fopen(path, "wb");
    if (fp == NULL) return -1;
    fwrite(data, size, 1, fp);
    fclose(fp);

    return 0;
}


extern int __CreateDirectory(const char* path)
{
    char dir[MAX_PATH];

    ConvertFullPath(path, dir, NULL);
    SetEnMark(dir);

    // Do not create directory if it already exists.
    {
        WIN32_FIND_DATA findData;
        HANDLE findHandle;

        findHandle = FindFirstFile(dir, &findData);
        if (findHandle != INVALID_HANDLE_VALUE)
        {
            FindClose(findHandle);
            return 0;
        }
    }

    {
        char* p;
        p = strchr(dir, '\\');
        while (p != NULL)
        {
            *p = '\0';
            CreateDirectory(dir, NULL);
            *p = '\\';

            p = strchr(p + 1, '\\');
        }
    }

    return 0;
}

extern int IsDirectory(const char* path)
{
    WIN32_FIND_DATA findData;
    HANDLE findHandle;

    findHandle = FindFirstFile(path, &findData);
    if (findHandle == INVALID_HANDLE_VALUE) return -1;
    FindClose(findHandle);

    return ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) ? 1 : 0;
}

extern int CreateFileInfo(const char* path, FileInfo* fileInfoBuffer)
{
    FileInfo* info;

    WIN32_FIND_DATA findData;
    HANDLE findHandle = INVALID_HANDLE_VALUE;
    int absDirLen, relDirLen;
    char relDir[1];
    char absDir[PATH_LENGTH];

    info = fileInfoBuffer;

    // Reset to some default data.
    {
        ConvertFullPath(path, absDir);
        DelChr(absDir, '\\');
        strrchr(absDir, '\\')[1] = '\0';
        absDirLen = strlen(absDir);
        relDir[0] = '\0';
        relDirLen = 0;
    }

    findHandle = FindFirstFile(path, &findData);
    if (findHandle == INVALID_HANDLE_VALUE) return -1;
    FindClose(findHandle);


    // Fill it with some data.
    memset(info, 0, sizeof(FileInfo));

    info->date.create = (((LONGLONG) findData.ftCreationTime.dwHighDateTime) << 32) + findData.ftCreationTime.dwLowDateTime;
    info->date.lastAccess = (((LONGLONG) findData.ftLastAccessTime.dwHighDateTime) << 32) + findData.ftLastAccessTime.dwLowDateTime;
    info->date.lastWrite = (((LONGLONG) findData.ftLastWriteTime.dwHighDateTime) << 32) + findData.ftLastWriteTime.dwLowDateTime;

    info->size = findData.nFileSizeLow;
    info->attributes = findData.dwFileAttributes;
    info->isDirectory = (u8) ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? 1 : 0);

    {
        int fileNameLen;
        fileNameLen = strlen(findData.cFileName);
        info->fileName = (char *) malloc((fileNameLen + 1) + (absDirLen + 1) + (relDirLen + 1));
        if (info->fileName == NULL) return -1;

        info->relDirectoryPath = info->fileName + fileNameLen + 1;
        info->absDirectoryPath = info->relDirectoryPath + relDirLen + 1;

        strcpy(info->fileName, findData.cFileName);
        strcpy(info->relDirectoryPath, relDir);
        strcpy(info->absDirectoryPath, absDir);
    }

    return 0;
}

extern int ReleaseFileInfo(FileInfo* fileInfo)
{
    if (fileInfo->fileName != NULL) free(fileInfo->fileName);
    fileInfo->fileName = NULL;

    return 0;
}

extern int SetFileTimestamp(const char* path, FileInfo* fileInfo)
{
    HANDLE hFile;
    FILETIME createTime, lastAccessTime, lastWriteTime;

    hFile = CreateFile(path,
            GENERIC_WRITE, 0, NULL,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        hFile = hFile;
    }

    createTime.dwHighDateTime = (u32) (fileInfo->date.create >> 32);
    createTime.dwLowDateTime = (u32) (fileInfo->date.create & 0xffffffff);
    lastAccessTime.dwHighDateTime = (u32) (fileInfo->date.lastAccess >> 32);
    lastAccessTime.dwLowDateTime = (u32) (fileInfo->date.lastAccess & 0xffffffff);
    lastWriteTime.dwHighDateTime = (u32) (fileInfo->date.lastWrite >> 32);
    lastWriteTime.dwLowDateTime = (u32) (fileInfo->date.lastWrite & 0xffffffff);

    SetFileTime(hFile, &createTime, &lastAccessTime, &lastWriteTime);
    CloseHandle(hFile);

    return 0;
}

extern int CompareFileTimestamp(FileInfo* finfo1, FileInfo* finfo2, bool create, bool lastAccess, bool lastWrite)
{
    if (create && finfo1->date.create != finfo2->date.create) return -1;
    if (lastAccess && finfo1->date.lastAccess != finfo2->date.lastAccess) return -1;
    if (lastWrite && finfo1->date.lastWrite != finfo2->date.lastWrite) return -1;
    return 0;
}


extern int CreateFileList(
    const char* directoryPath, FileInfoList* fileListInfo,
    int omitDirectory, int subDirectory,
    const char* omitName, const char* omitExName,
    const char* validExName)
{
    char dirPath[PATH_LENGTH];
    FileInfoList fileList;
    char* omitStrBuf = NULL, *omitExStrBuf = NULL, *validExStrBuf = NULL;
    char* omitStr[50], *omitExStr[50], *validExStr[100];

    ConvertFullPath(directoryPath, dirPath, NULL);

    {
        int res;

        DelChr(dirPath, '\\');

        res = IsDirectory(dirPath);
        if (res == -1) return -1;

        if (IsDirectory(dirPath) == 0)
        {
            fileListInfo->num = 1;

            fileListInfo->list = (FileInfo *) malloc(sizeof(FileInfo));
            CreateFileInfo(directoryPath, fileListInfo->list);

            return 1;
        }

        SetEnMark(dirPath);
    }

    fileList.num = 0;
    fileList.list = NULL;
    omitStrBuf = (char *) malloc(1024);
    if (omitStrBuf == NULL) goto err;
    omitExStrBuf = (char *) malloc(1024);
    if (omitExStrBuf == NULL) goto err;
    validExStrBuf = (char *) malloc(1024);
    if (validExStrBuf == NULL) goto err;

    // Omit name.
    if (omitName != NULL)
    {
        int i;
        char* p;

        strcpy(omitStrBuf, omitName);
        i = 0;
        p = omitStrBuf;
        omitStr[i] = p;
        i++;

        while ((p = strchr(p, ';')) != NULL)
        {
            *p = '\0';
            p++;
            omitStr[i] = p;
            i++;
        }
        omitStr[i] = NULL;
    }
    else
    {
        omitStr[0] = NULL;
    }

    // Omit ex name.
    if (omitExName != NULL)
    {
        int i;
        char* p;

        strcpy(omitExStrBuf, omitExName);
        i = 0;
        p = omitExStrBuf;
        omitExStr[i] = p;
        i++;

        while ((p = strchr(p, ';')) != NULL)
        {
            *p = '\0';
            p++;
            omitExStr[i] = p;
            i++;
        }
        omitExStr[i] = NULL;
    }
    else
    {
        omitExStr[0] = NULL;
    }

    // Valid ex name.
    if (validExName != NULL)
    {
        int i;
        char* p;

        strcpy(validExStrBuf, validExName);
        i = 0;
        p = validExStrBuf;
        validExStr[i] = p;
        i++;

        while ((p = strchr(p, ';')) != NULL)
        {
            *p = '\0';
            p++;
            validExStr[i] = p;
            i++;
        }
        validExStr[i] = NULL;
    }
    else
    {
        validExStr[0] = NULL;
    }


    // Do object enumeration twice, in order to check the amount of files
    // to be put into a buffer, and then fill up the data in the 2nd pass.
    fileList.num = EnumObject(dirPath, dirPath, NULL, omitDirectory, subDirectory, omitStr, omitStr, validExStr[0] != NULL ? validExStr : NULL);
    if (fileList.num < 0)
    {
        goto err;
    }

    fileList.list = (FileInfo *) malloc(fileList.num * sizeof(FileInfo));
    if (fileList.list == NULL) goto err;
    memset(fileList.list, 0, fileList.num * sizeof(FileInfo));

    fileList.num = 0;
    if (EnumObject(dirPath, dirPath, &fileList, omitDirectory, subDirectory, omitStr, omitExStr, validExStr[0] != NULL ? validExStr : NULL) < 0)
    {
        goto err;
    }
    if (fileListInfo != NULL) *fileListInfo = fileList;

    free(omitStrBuf);
    free(omitExStrBuf);
    free(validExStrBuf);

    return fileList.num;
err:
    ReleaseFileList(&fileList);
    if (omitStrBuf != NULL) free(omitStrBuf);
    if (omitExStrBuf != NULL) free(omitExStrBuf);
    if (validExStrBuf != NULL) free(validExStrBuf);

    fprintf(stderr, "%s: Error!!\n", __func__);

    return -1;
}

extern int ReleaseFileList(FileInfoList* directoryInfo)
{
    if (directoryInfo->list != NULL)
    {
        int i;
        FileInfo* finfo;

        finfo = directoryInfo->list;
        for (i = 0; i < directoryInfo->num; ++i, ++finfo)
        {
            if (finfo->fileName != NULL) free(finfo->fileName);
        }

        free(directoryInfo->list);
        directoryInfo->list = NULL;
    }
    memset(directoryInfo, 0, sizeof(FileInfoList));

    return 0;
}

extern int AnalyseFilePath(
    const char* src, char* fullPath, char* dirPath,
    char* fileName, char* name, char* exeName,
    const char* currentDir)
{
    char tmp_full[256], tmp_dir[256], tmp_fname[256], tmp_name[256], tmp_exename[256];

    ConvertFullPath(src, tmp_full, currentDir);
    AnalysisFileNameAndDirPath(tmp_full, tmp_fname, tmp_dir);
    AnalysisFileNameAndExeName(tmp_fname, tmp_name, tmp_exename);
    // Fill data.
    if (fullPath) strcpy(fullPath, tmp_full);
    if (dirPath) strcpy(dirPath, tmp_dir);
    if (fileName) strcpy(fileName, tmp_fname);
    if (name) strcpy(name, tmp_name);
    if (exeName) strcpy(exeName, tmp_exename);

    return 0;
}



extern int ConvertFullPath(const char* src, char* dest, const char* currentDir)
{
    int i, j, k;
    char iden[256], cur[MAX_PATH];

    if (currentDir == NULL)
    {
        GetCurrentDirectory(MAX_PATH, cur);
        currentDir = cur;
    }

    if (src == NULL)
    {
        strcpy(dest, currentDir);
        goto end;
    }

    i = 0;
    j = 0;
    k = 0;

    if ((src[0] == '\\' && src[1] == '\\') ||
        (src[0] == '/' && src[1] == '/'))
    {
        dest[0] = '\\';
        dest[1] = '\0';

        i += 2;
        j++;
    }
    else if (src[0] == '\\')
    {
        dest[0] = currentDir[0];
        dest[1] = currentDir[1];
        dest[2] = '\0';

        i++;
        j = 2;
    }
    else if (src[1] == ':')
    {
        dest[0] = src[0];
        dest[1] = src[1];
        dest[2] = '\0';

        i = 2;
        j = 2;

        if (src[i] == '\\') i++;
    }
    else
    {
        strcpy(dest, currentDir);
        j = strlen(dest);
        if (dest[j-1] == '\\' || dest[j-1] == '/')
        {
            dest[j-1] = '\0';
            j--;
        }
    }

    for (;;)
    {
        switch (src[i])
        {
            case '\0':
            {
                if (k != 0)
                {
                    dest[j] = '\\'; j++;
                    strcpy(&dest[j], iden);
                    j += k;
                }
                goto end;
            } break;

            case '\\':
            case '/':
            {
                if (k == 0)
                {
                    i++;
                    break;
                }
                if (strcmp(iden, ".") == 0) {
                    // Don't do anything here.
                } else if (strcmp(iden, "..") == 0) {
                    j--;
                    while (dest[j] != '\\' && dest[j] != '/' && dest[j] != ':') j--;
                    if (dest[j] != ':') dest[j] = '\0';
                    else j++;
                } else {
                    dest[j] = '\\'; j++;
                    strcpy(&dest[j], iden);
                    j += k;
                }

                k = 0;
                i++;
            } break;

            default:
            {
                if (_mbsbtype((unsigned char *)(&src[i]), 0) == 0)
                {
                    iden[k] = src[i];
                    iden[k+1] = 0;
                    k++;
                    i++;
                }
                else
                {
                    *((unsigned char *) &iden[k]) = *((unsigned short *) &src[i]);
                    iden[k+2] = '\0';
                    k += 2;
                    i += 2;
                }
            } break;
        }
    }

end:
    return 0;
}

extern int AnalysisFileNameAndDirPath(const char* src, char* fileName, char* dirPath)
{
    int i, last;

    i = 0;
    last = -1;
    while (src[i] != '\0')
    {
        if (_mbsbtype((const unsigned char *) &src[i], 0) == 0)
        {
            if (src[i] == '\\' || src[i] == '/' || src[i] == '\0' || src[i] == ':')
            {
                last = i;
            }
            i++;
        }
        else
        {
            i += 2;
        }
    }
    if (fileName != NULL)
    {
        if (last != -1) strcpy(fileName, &src[last+1]);
        else strcpy(fileName, src);
    }

    if (dirPath != NULL)
    {
        if (last != -1)
        {
            strncpy(dirPath, src, last);
            dirPath[last] = '\0';
        }
        else
        {
            dirPath[0] = '\0';
        }
    }

    return 0;
}

extern int AnalysisFileNameAndExeName(const char* src, char* name, char* exeName)
{
    char fileName[256], *p, ename[128], tmp_name[128];

    AnalysisFileNameAndDirPath(src, fileName, 0);

    if ((p = strrchr(fileName, '.')) == NULL)
    {
        strcpy(tmp_name, fileName);
        ename[0] = '\0';
    }
    else
    {
        strncpy(tmp_name, fileName, p - fileName);
        name[p - fileName] = '\0';
        strcpy(ename, p + 1);
    }

    if (name != NULL) strcpy(name, tmp_name);
    if (exeName != NULL) strcpy(exeName, ename);

    return 0;
}

// Get changed exe name path.
extern int GetChangeExeNamePath(const char* src, char* dest, const char* exeName)
{
    char dirPath[256], fileName[128];

    AnalysisFileNameAndDirPath(src, NULL, dirPath);
    AnalysisFileNameAndDirPath(src, fileName, 0);
    SetEnMark(dirPath);
    sprintf(dest, "%s%s.%s", dirPath, fileName, exeName);

    return 0;
}

// Some string utilities.
extern void SetEnMark(char* pathBuf)
{
    int len = (int) strlen(pathBuf);

    if (pathBuf[len-1] != '\\')
    {
        pathBuf[len] = '\\';
        pathBuf[len+1] = '\0';
    }
}

extern void SetChr(char* pathBuf, char ch)
{
    int len = (int) strlen(pathBuf);

    if (pathBuf[len-1] != ch)
    {
        pathBuf[len] = ch;
        pathBuf[len+1] = '\0';
    }
}
extern void DelChr(char* pathBuf, char ch)
{
    int len = (int) strlen(pathBuf);

    if (pathBuf[len-1] == ch)
    {
        pathBuf[len-1] = '\0';
    }
}

extern int GetExName(const char* path, char* exNameBuf)
{
    char* p;

    p = strrchr(const_cast<char *>(path), '.');
    if (p == NULL) exNameBuf[0] = '\0';
    else strcpy(exNameBuf, p+1);

    return 0;
}

extern int SetExName(const char* path, char* exName, char* destBuf)
{
    char* p;
    char tempStr[256];

    strcpy(tempStr, path);

    p = strrchr(tempStr, '.');
    if (p == NULL)
    {
        sprintf(destBuf, "%s.%s", tempStr, exName);
    }
    else
    {
        strncpy(destBuf, tempStr, p - tempStr + 1);
        strcpy(destBuf + (p - tempStr + 1), exName);
    }

    return 0;
}

extern int CheckTextData(void* buffer, int size)
{
    unsigned char* p;
    int len, search;

    search = size > TEXTCHECKSIZE ? TEXTCHECKSIZE : size;

    {
        int con;

        p = reinterpret_cast<unsigned char *> (buffer);
        con = 0;
        for (len = search; len > 0; len--, p++)
        {
            if (*p == '\0')
            {
                con++;
                if (con >= 2) return 0;
            }
        }
    }

    {
        int con;

        p = reinterpret_cast<unsigned char *> (buffer);
        con = 0;
        for (len = search; len > 0; len--, p++)
        {
            if (*p < 0x20 && *p != '\r' && *p != '\n' && *p != '\t')
            {
                con++;
                if (con >= 1) return 0;
            }
        }
    }

    {
        p = reinterpret_cast<unsigned char *> (buffer);
        for (len = search; len > 0; len--, p++)
        {
            if ((*p >= 0x81 && *p <= 0x9f) || (*p >= 0xe0 && *p <= 0xfc))
            {
                len--;
                p++;
                if (len <= 0 && size <= search) return 0;

                if (!((*p >= 0x40 && *p <= 0x7e) || (*p >= 0x80 && *p <= 0xfc)))
                {
                    return 0;
                }

                if (len == 0) return 1;
            }
        }
    }

    return 1;
}

extern int CheckTextFile(const char* path)
{
    void* buf;
    int size, res;
    FILE* fp;

    fp = fopen(path, "rb");
    if (fp == NULL) return -1;

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // Clamp size.
    size = size > TEXTCHECKSIZE ? TEXTCHECKSIZE : size;

    buf = malloc(size);
    if (buf == NULL)
    {
        fclose(fp);
        return -1;
    }

    fread(buf, size, 1, fp);
    fclose(fp);

    res = CheckTextData(buf, size);
    free(buf);

    return res;
}


/*
   Does it really have multiply bytes?
*/
extern int CheckMultiByteChar(char* buf)
{
    return (static_cast<unsigned char> (*buf) >= 0x81 && static_cast<unsigned char> (*buf) >= 0xE0 && static_cast<unsigned char> (*buf) <= 0xFC);
}
