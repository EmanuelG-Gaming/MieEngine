#include "base_log.h"
#include "base_defs.h"
#include "base_string.h"

#include <stdarg.h>
#include <string.h>

#include <stdio.h>


static THREAD_LOCAL LOGCONTEXT logContext = { 0 };

extern void LogFrameBegin(void)
{
    // Lazy initialization.
    if (logContext.arena == NULL)
    {
        logContext.arena = ArenaInit(MB(8), KB(8), ARENA_FLAG_GROWABLE);
    }

    LOGFRAME* frame = ArenaPushStruct(logContext.arena, LOGFRAME);
    frame->arenaStartPos = ArenaGetPos(logContext.arena);

    // Chain this in a linked list.
    SLL_STACK_PUSH(logContext.stack, frame);
}

extern int LogFramePeekCount(u32 levelMask)
{
    if (logContext.arena == NULL)
    {
        return 0;
    }

    LOGFRAME* frame = logContext.stack;

    if (levelMask == LOG_ALL)
    {
        return frame->nLogs;
    }

    u32 count = 0;
    for (LOGMSG* msg = frame->first; msg != NULL; msg = msg->next)
    {
        // Extract all messages that are within the level mask.
        if (msg->level & levelMask)
        {
            count++;
        }
    }

    return count;
}

// The array designated initializer was a C99 feature that was only added in C++ in C++20.
static const String8 levelPrefixes[] = {
    STR8_CONST_LIT("PAD0"),
    STR8_CONST_LIT("[Info] "),
    STR8_CONST_LIT("[Warn] "),
    STR8_CONST_LIT("PAD3"),
    STR8_CONST_LIT("[Error] "),
    STR8_CONST_LIT("[UNKNOWN] "),
};

#define LOG_PREFIX_INDEX(level) \
    MIN(sizeof(levelPrefixes) / sizeof(levelPrefixes[0]) - 1, (u32)(level))



extern String8 LogFramePeek(
    ARENA *arena, u32 levelMask,
    int resType, b32 prefixLevel)
{
    if (logContext.arena == NULL)
    {
        return CLITERAL(String8) { 0 };
    }

    LOGFRAME* frame = logContext.stack;

    String8 out = { 0 };

    if (resType == LOG_RES_CONCAT)
    {
        u32 nLogs = 0;
        u64 outputSize = 0;

        // Iterate over linked list.
        for (LOGMSG* currentLog = frame->first;
            currentLog != NULL; currentLog = currentLog->next)
        {
            if ((currentLog->level & levelMask) == 0)
            {
                continue;
            }

            nLogs++;

            u32 prefixIndex = LOG_PREFIX_INDEX(currentLog->level);
            String8 prefix = prefixLevel ? levelPrefixes[prefixIndex] : CLITERAL(String8) { 0 };

            outputSize += prefix.len;
            outputSize += currentLog->msg.len;
        }

        if (nLogs > 0)
        {
            outputSize += nLogs - 1;

            out.len = outputSize;
            out.data = ArenaPushArrayZero(arena, u8, out.len);

            u64 outPos = 0;

            for (LOGMSG* currentLog = frame->first;
                currentLog != NULL; currentLog = currentLog->next)
            {
                // Do memcpy.
                if ((currentLog->level & levelMask) == 0)
                {
                    continue;
                }

                nLogs++;

                u32 prefixIndex = LOG_PREFIX_INDEX(currentLog->level);
                String8 prefix = prefixLevel ? levelPrefixes[prefixIndex] : CLITERAL(String8) { 0 };

                Str8_memcpy(&out, &prefix, outPos);
                outPos += prefix.len;

                Str8_memcpy(&out, &currentLog->msg, outPos);
                outPos += currentLog->msg.len;

                if (outPos < out.len)
                {
                    // The concatenated char.
                    out.data[outPos++] = LOG_CONCAT_CHAR;
                }
            }
        }
    }
    else
    {
        LOGMSG* selectedLog = NULL;

        if (resType == LOG_RES_FIRST)
        {
            LOGMSG* currentLog = frame->first;

            // Displace current log until we meet the criteria.
            while (currentLog != NULL && (currentLog->level & levelMask) == 0)
            {
                currentLog = currentLog->next;
            }

            selectedLog = currentLog;
        }
        else if (resType == LOG_RES_LAST)
        {
            LOGMSG* currentLog = frame->first;
            LOGMSG* lastValid = NULL;

            while (currentLog != NULL)
            {
                if (currentLog->level & levelMask)
                {
                    lastValid = currentLog;
                }

                currentLog = currentLog->next;
            }

            selectedLog = lastValid;
        }

        // Check after selecting.
        if (selectedLog != NULL)
        {
            String8 msg = selectedLog->msg;
            u32 prefixIndex = LOG_PREFIX_INDEX(selectedLog->level);
            String8 prefix = prefixLevel ? levelPrefixes[prefixIndex] : CLITERAL(String8) { 0 };

            out.len = msg.len + prefix.len;
            out.data = ArenaPushArrayZero(arena, u8, out.len);

            // TODO: uCRT bloat?
            memcpy(out.data, prefix.data, prefix.len);
            memcpy(out.data + prefix.len, msg.data, msg.len);
        }
    }

    return out;
}

extern String8 LogFrameEnd(ARENA *arena, u32 levelMask, int resType, b32 prefixLevel)
{
    if (logContext.arena == NULL)
    {
        return CLITERAL(String8) { 0 };
    }

    String8 out = LogFramePeek(arena, levelMask, resType, prefixLevel);
    ArenaPopTo(logContext.arena, logContext.stack->arenaStartPos);
    SLL_STACK_POP(logContext.stack);

    return out;
}


static void LogEmitImpl(int logLevel, String8 msg)
{
    LOGFRAME* frame = logContext.stack;

    LOGMSG* currentLog = ArenaPushStruct(logContext.arena, LOGMSG);
    currentLog->msg = msg;
    currentLog->level = logLevel;

    frame->nLogs++;
    SLL_APPEND_BACK(frame->first, frame->last, currentLog);
}

extern void LogEmit(int logLevel, String8 originalMsg)
{
    //fprintf(stderr, "%s: Ok.\n", __func__);
    if (logContext.arena == NULL)
    {
        return;
    }

    String8 msg = Str8_copy(logContext.arena, originalMsg);
    LogEmitImpl(logLevel, msg);
}

extern void LogEmitF(int logLevel, const char *fmt, ...)
{
    if (logContext.arena == NULL)
    {
        return;
    }

    va_list args;
    va_start(args, fmt);

    String8 msg = Str8_pushfv(logContext.arena, fmt, args);

    va_end(args);

    LogEmitImpl(logLevel, msg);
}
