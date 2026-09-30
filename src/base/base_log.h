#ifndef BASE_LOG_H_
#define BASE_LOG_H_ 1

#include "base_defs.h"
#include "base_string.h"

#include "../mem/arena.h"

/*
   Log levels.
*/

#define LOG_NONE 0
#define LOG_INFO 1
#define LOG_WARN 2
#define LOG_ERROR 4

#define LOG_ALL ~((u32) 0)


/*
   Log subresource handling.
*/

#define LOG_RES_NONE 0
#define LOG_RES_CONCAT 1
#define LOG_RES_FIRST 2
#define LOG_RES_LAST 4


#define LOG_CONCAT_CHAR '\n'

/*
   Chain log messages into a linked list.
*/

// Chain of log messages.
typedef struct LOGMSG {
    struct LOGMSG* next;
    String8 msg;
    int level;
} LOGMSG;

// Chain of log frames.
typedef struct LOGFRAME {
    struct LOGFRAME* next;

    u64 arenaStartPos;

    u32 nLogs;
    LOGMSG* first;
    LOGMSG* last;
} LOGFRAME;

typedef struct LOGCONTEXT {
    ARENA* arena;
    // Log Stack.
    LOGFRAME* stack;
} LOGCONTEXT;


// Different log levels.

#define LogInfo(msg) LogEmit(LOG_INFO, STR8_LIT(msg))
#define LogInfoStr(str) LogEmit(LOG_INFO, (str))
#define LogInfoEmitF(fmt, ...) LogEmitF(LOG_INFO, (fmt), __VA_ARGS__)

#define LogWarn(msg) LogEmit(LOG_WARN, STR8_LIT(msg))
#define LogWarnStr(str) LogEmit(LOG_WARN, (str))
#define LogWarnEmitF(fmt, ...) LogEmitF(LOG_WARN, (fmt), __VA_ARGS__)

#define LogError(msg) LogEmit(LOG_ERROR, STR8_LIT(msg))
#define LogErrorStr(str) LogEmit(LOG_ERROR, (str))
#define LogErrorEmitF(fmt, ...) LogEmitF(LOG_ERROR, (fmt), __VA_ARGS__)


/*
   Log frame utilities.
*/

extern void LogFrameBegin(void);
// Iterate over the linked list.
extern int LogFramePeekCount(u32 levelMask);

extern String8 LogFramePeek(ARENA* arena, u32 levelMask, int resType, b32 prefixLevel);

extern String8 LogFrameEnd(ARENA* arena, u32 levelMask, int resType, b32 prefixLevel);


/*
   Log emitting.
*/

extern void LogEmit(int logLevel, String8 msg);
extern void LogEmitF(int logLevel, const char* fmt, ...);



#endif /* BASE_LOG_H_ */
