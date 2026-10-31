#ifndef NASE_UTILS_H_
#define BASE_UTILS_H_ 1


//#define LOG_BASIC
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
#endif /* BASE_UTILS_H_ */
