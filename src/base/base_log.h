#ifndef BASE_LOG_H_
#define BASE_LOG_H_ 1

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>



#ifdef LOG_BASIC
    #define LOG_NON_COLOR
    #define LOG_NON_STYLE
#endif

#ifdef LOG_NON_COLOR
    #define LOG_NON_FG
    #define LOG_NON_BG
#else
    #define LOG_CDEF(T) T
#endif

#ifndef LOG_NON_BG
    #define LOG_BG_DEF(T) T
#else
    #define LOG_BG_DEF(T)
#endif

#ifndef LOG_NON_FG
    #define LOG_FG_DEF(T) T
#else
    #define LOG_FG_DEF(T)
#endif

#ifndef LOG_NON_STYLE
    #define LOG_SDEF(T) T
#else
    #define LOG_SDEF(T)
#endif


#ifndef LOG_BASIC
    #define LOG_COLOR_RESET "\033[0m"
#else
    #define LOG_COLOR_RESET
#endif

/*
   Foregrounds.
*/

#define LOG_FG_BLACK LOG_FG_DEF("\033[30m")
#define LOG_FG_RED LOG_FG_DEF("\033[31m")
#define LOG_FG_GREEN LOG_FG_DEF("\033[32m")
#define LOG_FG_YELLOW LOG_FG_DEF("\033[33m")
#define LOG_FG_BLUE LOG_FG_DEF("\033[34m")
#define LOG_FG_MAGENTA LOG_FG_DEF("\033[35m")
#define LOG_FG_CYAN LOG_FG_DEF("\033[36m")
#define LOG_FG_WHITE LOG_FG_DEF("\033[37m")
#define LOG_FG_GRAY LOG_FG_DEF("\033[90m")

#define LOG_FG_BRIGHT_BLACK LOG_FG_DEF("\033[90m")
#define LOG_FG_BRIGHT_RED LOG_FG_DEF("\033[91m")
#define LOG_FG_BRIGHT_GREEN LOG_FG_DEF("\033[92m")
#define LOG_FG_BRIGHT_YELLOW LOG_FG_DEF("\033[93m")
#define LOG_FG_BRIGHT_BLUE LOG_FG_DEF("\033[94m")
#define LOG_FG_BRIGHT_MAGENTA LOG_FG_DEF("\033[95m")
#define LOG_FG_BRIGHT_CYAN LOG_FG_DEF("\033[96m")
#define LOG_FG_BRIGHT_WHITE LOG_FG_DEF("\033[97m")

/*
   Backgrounds.
*/

#define LOG_BG_BLACK LOG_BG_DEF("\033[40m")
#define LOG_BG_RED LOG_BG_DEF("\033[41m")
#define LOG_BG_GREEN LOG_BG_DEF("\033[42m")
#define LOG_BG_YELLOW LOG_BG_DEF("\033[43m")
#define LOG_BG_BLUE LOG_BG_DEF("\033[44m")
#define LOG_BG_MAGENTA LOG_BG_DEF("\033[45m")
#define LOG_BG_CYAN LOG_BG_DEF("\033[46m")
#define LOG_BG_WHITE LOG_BG_DEF("\033[47m")
#define LOG_BG_GRAY LOG_BG_DEF("\033[100m")

#define LOG_BG_BRIGHT_RED LOG_BG_DEF("\033[101m")
#define LOG_BG_BRIGHT_GREEN LOG_BG_DEF("\033[102m")
#define LOG_BG_BRIGHT_YELLOW LOG_BG_DEF("\033[103m")
#define LOG_BG_BRIGHT_BLUE LOG_BG_DEF("\033[104m")
#define LOG_BG_BRIGHT_MAGENTA LOG_BG_DEF("\033[105m")
#define LOG_BG_BRIGHT_CYAN LOG_BG_DEF("\033[106m")
#define LOG_BG_BRIGHT_WHITE LOG_BG_DEF("\033[107m")


/*
   Text styles.
*/

#define LOG_STYLE_BOLD LOG_SDEF("\033[1m")
#define LOG_STYLE_DIM LOG_SDEF("\033[2m")
#define LOG_STYLE_ITALIC LOG_SDEF("\033[3m")
#define LOG_STYLE_UNDERLINE LOG_SDEF("\033[4m")
#define LOG_STYLE BLINK LOG_SDEF("\033[5m")
#define LOG_STYLE_INVERT LOG_SDEF("\033[7m")
#define LOG_STYLE_HIDDEN LOG_SDEF("\033[8m")
#define LOG_STYLE_STRIKETHROUGH LOG_SDEF("\033[9m")

// The log levels are sorted by their importance,
// with the downmost levels appearing below or equal the minimum level.
typedef enum LOGLEVEL {
    LOG_DEBUG,
    LOG_INFO,
    LOG_SUCCESS,
    LOG_FAIL,
    LOG_WARN,
    LOG_FIXME,
    LOG_ERROR,
    LOG_FATAL,

    LOG_N,
} LOGLEVEL;

typedef enum LOGFLAGS {
    LOG_FLAG_NONE = 0,
    LOG_FLAG_FUNC = (1 << 0),
    LOG_FLAG_TIME = (1 << 1),
    LOG_FLAG_PREFIX = (1 << 2),
    LOG_FLAG_ALIGNED = (1 << 3),
    LOG_FLAG_CANCER = (1 << 4),
} LOGFLAGS;

static LOGLEVEL logMinLevel = LOG_DEBUG;

static const unsigned int logDefaultFlag =
    (LOG_FLAG_FUNC | LOG_FLAG_TIME | LOG_FLAG_PREFIX | LOG_FLAG_ALIGNED | LOG_FLAG_CANCER);


static const char* LogGetLevelPrefix(LOGLEVEL level)
{
    switch (level) {
        case LOG_INFO: return LOG_STYLE_BOLD LOG_FG_CYAN "[INFO]" LOG_COLOR_RESET;
        case LOG_SUCCESS: return LOG_STYLE_BOLD LOG_FG_GREEN "[SUCCESS]" LOG_COLOR_RESET;
        case LOG_FAIL: return LOG_STYLE_BOLD LOG_FG_RED "[FAIL]" LOG_COLOR_RESET;
        case LOG_WARN: return LOG_STYLE_BOLD LOG_FG_YELLOW "[WARN]" LOG_COLOR_RESET;
        case LOG_FIXME: return LOG_STYLE_BOLD LOG_STYLE_ITALIC LOG_STYLE_UNDERLINE LOG_FG_YELLOW "[FIXME]" LOG_COLOR_RESET;
        case LOG_ERROR: return LOG_STYLE_BOLD LOG_STYLE_ITALIC LOG_STYLE_UNDERLINE LOG_FG_RED "[ERROR]" LOG_COLOR_RESET;
        case LOG_FATAL: return LOG_STYLE_BOLD LOG_STYLE_ITALIC LOG_STYLE_UNDERLINE LOG_BG_RED LOG_FG_BLACK "[FATAL]" LOG_COLOR_RESET;
        case LOG_DEBUG: return LOG_FG_GRAY "[DEBUG]" LOG_COLOR_RESET;

        default: return "";
    }
}

static const char* LogGetLevelPrefix_Cancer(LOGLEVEL level)
{
    switch (level) {
        case LOG_INFO: return LOG_STYLE_BOLD LOG_FG_CYAN "[INFO 💭]" LOG_COLOR_RESET;
        case LOG_SUCCESS: return LOG_STYLE_BOLD LOG_FG_GREEN "[SUCCESS ✅]" LOG_COLOR_RESET;
        case LOG_FAIL: return LOG_STYLE_BOLD LOG_FG_RED "[FAIL ❌]" LOG_COLOR_RESET;
        case LOG_WARN: return LOG_STYLE_BOLD LOG_FG_YELLOW "[WARN ⚠ ]" LOG_COLOR_RESET;
        case LOG_FIXME: return LOG_STYLE_BOLD LOG_STYLE_ITALIC LOG_STYLE_UNDERLINE LOG_FG_YELLOW "[FIXME 🛠]" LOG_COLOR_RESET;
        case LOG_ERROR: return LOG_STYLE_BOLD LOG_STYLE_ITALIC LOG_STYLE_UNDERLINE LOG_FG_RED "[ERROR 💥]" LOG_COLOR_RESET;
        case LOG_FATAL: return LOG_STYLE_BOLD LOG_STYLE_ITALIC LOG_STYLE_UNDERLINE LOG_BG_RED LOG_FG_BLACK "[FATAL 💀]" LOG_COLOR_RESET;
        case LOG_DEBUG: return LOG_FG_GRAY "[DEBUG ➖]" LOG_COLOR_RESET;

        default: return "";
    }
}

static const char* LogGetLevelPrefixEx(LOGLEVEL level, int cancer)
{
    if (cancer) return LogGetLevelPrefix_Cancer(level);
    else return LogGetLevelPrefix(level);
}


static int LogGetLevelPrefixLength(LOGLEVEL level)
{
    switch (level) {
        case LOG_INFO: return 6;
        case LOG_SUCCESS: return 9;
        case LOG_FAIL: return 6;
        case LOG_WARN: return 6;
        case LOG_FIXME: return 7;
        case LOG_ERROR: return 7;
        case LOG_FATAL: return 7;
        case LOG_DEBUG: return 7;

        default: return 0;
    }
}


/*
   Functions that accept C strings.
*/


static void LogAtLevelEx(LOGLEVEL level, unsigned int flags, const char* funcName, const char* format, ...)
{
    if (level < logMinLevel) {
        return;
    }

    va_list args;
    va_start(args, format);

    char buf[1024];
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    // Build the log line.
    char logLine[2048];

    if (flags & LOG_FLAG_TIME) {
        time_t now;
        time(&now);
        char* timeStr = ctime(&now);
        timeStr[strlen(timeStr)-1] = '\0'; // Remove newline.
        snprintf(logLine, sizeof(logLine), LOG_FG_GRAY "[%s]" LOG_COLOR_RESET " ", timeStr);
    }

    if (flags & LOG_FLAG_PREFIX) {
        const char* prefix = LogGetLevelPrefixEx(level, (flags & LOG_FLAG_CANCER));
        snprintf(logLine + strlen(logLine),
                sizeof(logLine) - strlen(logLine), "%s ", prefix);
    }

    if (flags & LOG_FLAG_FUNC) {
        snprintf(logLine + strlen(logLine), sizeof(logLine) - strlen(logLine), "%s:", funcName);
    }

    /*
    if (flags & LOG_FLAG_FUNC) {
        fprintf(stderr, "%s %s: %s", prefix, funcName, buf);
    } else {
        fprintf(stderr, "%s %s", prefix, buf);
    }
    */
    fprintf(stderr, "%s %s", logLine, buf);
    fflush(stderr);
}

#define LogAtLevelF(level, funcName, format, ...) LogAtLevelEx(level, logDefaultFlag, funcName, format __VA_ARGS__)
#define LogAtLevel(level, format, ...) LogAtLevelEx(level, logDefaultFlag, __func__, format __VA_ARGS__)

#define LogInfo(format, ...) LogAtLevel(LOG_INFO, format __VA_ARGS__)
#define LogSuccess(format, ...) LogAtLevel(LOG_SUCCESS, format __VA_ARGS__)
#define LogFail(format, ...) LogAtLevel(LOG_FAIL, format __VA_ARGS__)
#define LogWarn(format, ...) LogAtLevel(LOG_WARN, format __VA_ARGS__)
#define LogFixme(format, ...) LogAtLevel(LOG_FIXME, format __VA_ARGS__)
#define LogError(format, ...) LogAtLevel(LOG_ERROR, format __VA_ARGS__)
#define LogFatal(format, ...) LogAtLevel(LOG_FATAL, format __VA_ARGS__)
#define LogDebug(format, ...) LogAtLevel(LOG_DEBUG, format __VA_ARGS__)



#endif /* BASE_LOG_H_ */
