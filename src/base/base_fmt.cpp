#include "base_fmt.h"
#include "base_defs.h"

#include "base_log.h"

#include <stdarg.h>

typedef enum FormatValueType {
    FMT_VT_NONE = 0,

    FMT_VT_I8,
    FMT_VT_I16,
    FMT_VT_I32,
    FMT_VT_I64,

    FMT_VT_U8,
    FMT_VT_U16,
    FMT_VT_U32,
    FMT_VT_U64,

    FMT_VT_B8,
    FMT_VT_B32,

    FMT_VT_F32,
    FMT_VT_F64,

    FMT_VT_CSTR,
    FMT_VT_STR8,

    FMT_VT_V2I16,
    FMT_VT_V2I32,
    FMT_VT_V2F32,

    FMT_VT_V3F32,
    FMT_VT_V4F32,

    FMT_VT_POINTER,
} FormatValueType;

typedef enum FormatType {
    FMT_FT_NONE = 0,

    FMT_FT_DECIMAL,
    FMT_FT_LOWER_SCIENTIFIC,
    FMT_FT_UPPER_SCIENTIFIC,
    FMT_FT_LOWER_ADAPT,
    FMT_FT_UPPER_ADAPT,
    FMT_FT_LOWER_HEX,
    FMT_FT_UPPER_HEX,

    FMT_FT_OCTAL,
    FMT_FT_BINARY,
    FMT_FT_BOOLEAN,
    FMT_FT_POINTER,
    FMT_FT_CODEPOINT,
} FormatType;

typedef enum FormatFlags {
    FMT_FLAG_LEFT_JUSTIFY = (1 << 0),
    FMT_FLAG_CENTER_JUSTIFY = (1 << 1),
    FMT_FLAG_ALWAYS_SIGN = (1 << 2),
    FMT_FLAG_POSITIVE_SPACE = (1 << 3),
    FMT_FLAG_PAD_ZERO = (1 << 4),
    FMT_FLAG_ADD_COMMAS = (1 << 5),
} FormatFlags;

typedef struct FormatSpecifier {
    FormatValueType valueType;
    FormatType formatType;

    u32 minWidth;
    u32 minDecimal;

    u32 flags;

    b32 valid;
} FormatSpecifier;

// Pushes character into u8* char array.
#define FMT_TRY_PUSH(c) do { \
    if (pos < cap) { out[pos] = c; } \
    pos++; \
} while (0)


static const char* ValTypeToStr(int valueType)
{
    switch (valueType)
    {
        case FMT_VT_I8: return "i8";
        case FMT_VT_I16: return "i16";
        case FMT_VT_I32: return "i32";
        case FMT_VT_I64: return "i64";
        case FMT_VT_U8: return "u8";
        case FMT_VT_U16: return "u16";
        case FMT_VT_U32: return "u32";
        case FMT_VT_U64: return "u64";
        case FMT_VT_B8: return "b8";
        case FMT_VT_B32: return "b32";
        case FMT_VT_F32: return "f32";
        case FMT_VT_F64: return "f64";
        case FMT_VT_CSTR: return "cstr";
        case FMT_VT_STR8: return "str8";
        case FMT_VT_V2I16: return "v2i16";
        case FMT_VT_V2I32: return "v2i32";

        case FMT_VT_V2F32: return "v2f32";
        case FMT_VT_V3F32: return "v3f32";
        case FMT_VT_V4F32: return "v4f32";


        case FMT_VT_POINTER: return "ptr";

        default: return "UNKNOWN VALUE TYPE";
    }
}

static FormatValueType ParseValueType(String8 typeStr)
{
    if (typeStr.len == 0)
    {
        return FMT_VT_NONE;
    }

    if (typeStr.data[typeStr.len-1] == '*')
    {
        // We have a pointer.
        return FMT_VT_POINTER;
    }

    if (Str8_equals(STR8_LIT("i8"), typeStr)) {
        return FMT_VT_I8;
    } else if (Str8_equals(STR8_LIT("i16"), typeStr)) {
        return FMT_VT_I16;
    } else if (Str8_equals(STR8_LIT("i32"), typeStr)) {
        return FMT_VT_I32;
    } else if (Str8_equals(STR8_LIT("i64"), typeStr)) {
        return FMT_VT_I64;
    } else if (Str8_equals(STR8_LIT("u8"), typeStr)) {
        return FMT_VT_U8;
    } else if (Str8_equals(STR8_LIT("u16"), typeStr)) {
        return FMT_VT_U16;
    } else if (Str8_equals(STR8_LIT("u32"), typeStr)) {
        return FMT_VT_U32;
    } else if (Str8_equals(STR8_LIT("u64"), typeStr)) {
        return FMT_VT_U64;
    } else if (Str8_equals(STR8_LIT("b8"), typeStr)) {
        return FMT_VT_B8;
    } else if (Str8_equals(STR8_LIT("b32"), typeStr)) {
        return FMT_VT_B32;
    } else if (Str8_equals(STR8_LIT("cstring"), typeStr)) {
        return FMT_VT_CSTR;
    } else if (Str8_equals(STR8_LIT("string8"), typeStr)) {
        return FMT_VT_STR8;
    } else if (Str8_equals(STR8_LIT("v2i16"), typeStr)) {
        return FMT_VT_V2I16;
    } else if (Str8_equals(STR8_LIT("v2i32"), typeStr)) {
        return FMT_VT_V2I32;
    } else if (Str8_equals(STR8_LIT("v2f32"), typeStr)) {
        return FMT_VT_V2F32;
    } else if (Str8_equals(STR8_LIT("v3f32"), typeStr)) {
        return FMT_VT_V3F32;
    } else if (Str8_equals(STR8_LIT("v4f32"), typeStr)) {
        return FMT_VT_V4F32;
    }

    // Otherwise, it's a pointer.
    return FMT_VT_POINTER;
}

static b32 ParseSettings(FormatSpecifier* out, String8 settings, va_list args)
{
    b8 expectingDecimalWidth = FALSE;
    b8 parsingNum = FALSE;
    u32 currentNum = 0;

    for (size_t i = 0; i < settings.len; ++i)
    {
        u8 c = settings.data[i];

        if ('1' <= c && c <= '9')
        {
            // We have a number here.
            if (!parsingNum)
            {
                parsingNum = TRUE;
                currentNum = 0;
            }

            currentNum *= 10;
            currentNum += c - '0';

            continue;
        }
        else if (parsingNum)
        {
            // We were parsing the number.
            parsingNum = FALSE;

            if (expectingDecimalWidth)
            {
                out->minDecimal = currentNum;
            }
            else
            {
                out->minWidth = currentNum;
            }
        }
        else if (expectingDecimalWidth && c != '*')
        {
            // We were expecting a decimal width, but none were given.
            return FALSE;
        }


        switch (c)
        {
            case 'd': { out->formatType = FMT_FT_DECIMAL; } break;
            case 'e': { out->formatType = FMT_FT_LOWER_SCIENTIFIC; } break;
            case 'E': { out->formatType = FMT_FT_UPPER_SCIENTIFIC; } break;
            case 'f': { out->formatType = FMT_FT_LOWER_ADAPT; } break;
            case 'F': { out->formatType = FMT_FT_UPPER_ADAPT; } break;
            case 'x': { out->formatType = FMT_FT_LOWER_HEX; } break;
            case 'X': { out->formatType = FMT_FT_UPPER_HEX; } break;
            case 'o': { out->formatType = FMT_FT_OCTAL; } break;
            case 'b': { out->formatType = FMT_FT_BINARY; } break;
            case 'B': { out->formatType = FMT_FT_BOOLEAN; } break;
            case 'p': { out->formatType = FMT_FT_POINTER; } break;
            case 'c': { out->formatType = FMT_FT_CODEPOINT; } break;

            case '-': { out->flags |= FMT_FLAG_LEFT_JUSTIFY; } break;
            case '|': { out->flags |= FMT_FLAG_CENTER_JUSTIFY; } break;
            case '+': { out->flags |= FMT_FLAG_ALWAYS_SIGN; } break;
            case ' ': { out->flags |= FMT_FLAG_POSITIVE_SPACE; } break;
            case '0': { out->flags |= FMT_FLAG_PAD_ZERO; } break;
            case '\'': { out->flags |= FMT_FLAG_ADD_COMMAS; } break;

            case '.': { expectingDecimalWidth = TRUE; } break;

            case '*':
            {
                u32 value = va_arg(args, u32);

                if (expectingDecimalWidth)
                {
                    expectingDecimalWidth = FALSE;
                    out->minDecimal = value;
                }
                else
                {
                    out->minWidth = value;
                }
            } break;

            default: { return FALSE; }
        }
    }

    if (parsingNum)
    {
        if (expectingDecimalWidth)
        {
            out->minDecimal = currentNum;
        }
        else
        {
            out->minWidth = currentNum;
        }
    }

    return TRUE;
}

static FormatSpecifier ParseSpecifier(String8 spec, va_list args)
{
    FormatSpecifier out;
    out.flags = 0;
    out.formatType = FMT_FT_NONE;
    out.valueType = FMT_VT_NONE;
    out.minDecimal = 0;
    out.minWidth = 0;
    out.valid = FALSE;


    if (spec.len <= 2 || spec.data[0] != '{' || spec.data[spec.len-1] != '}')
    {
        //goto invalid;
        return out;
    }

    spec = Str8_substr(spec, 1, spec.len - 1);
    u64 colonI = Str8_findFirst(spec, ':');

    // Delimit the string into type:settings.
    String8 typeStr = Str8_substr(spec, 0, colonI);
    String8 settingsStr = Str8_substr(spec, colonI+1, spec.len);

    out.valueType = ParseValueType(typeStr);

    switch (out.valueType)
    {
        case FMT_VT_NONE: goto invalid;

        case FMT_VT_I8:
        case FMT_VT_I16:
        case FMT_VT_I32:
        case FMT_VT_I64:
        case FMT_VT_U8:
        case FMT_VT_U16:
        case FMT_VT_U32:
        case FMT_VT_U64:

        case FMT_VT_V2I16:
        case FMT_VT_V2I32:
        {
            out.formatType = FMT_FT_DECIMAL;
        } break;

        case FMT_VT_B8:
        case FMT_VT_B32:
        {
            out.formatType = FMT_FT_BOOLEAN;
        } break;

        case FMT_VT_F32:
        case FMT_VT_F64:
        case FMT_VT_V2F32:
        case FMT_VT_V3F32:
        case FMT_VT_V4F32:
        {
            out.formatType = FMT_FT_LOWER_ADAPT;
        } break;

        case FMT_VT_POINTER:
        {
            out.formatType = FMT_FT_POINTER;
        } break;

        // Strings do not get a format type.
        case FMT_VT_CSTR:
        case FMT_VT_STR8:
        {
            out.formatType = FMT_FT_NONE;
        } break;
    }

    if (!ParseSettings(&out, settingsStr, args))
    {
        goto invalid;
    }

    out.valid = TRUE;

invalid:
    return out;
}


static u64 Str8_formatv_impl(String8 fmt, va_list args, u8* out, u64 cap)
{
    u64 pos = 0;

    u64 specifierStart = 0;
    b8 inSpecifier = FALSE;

    for (size_t i = 0; i < fmt.len; ++i)
    {
        u8 c = fmt.data[i];

        if (inSpecifier)
        {
            // Check ending first.
            if (c == '}')
            {
                inSpecifier = FALSE;

                String8 specifierStr = Str8_substr(fmt, specifierStart, i+1);
                FormatSpecifier specifier = ParseSpecifier(specifierStr, args);

                // Some debug information.
                /*
                */
                LogInfoEmitF("%u | VT: %s, FT: %2u, width: %2u, decimal: %u, flags: 0x%x",
                    specifier.valid,
                    ValTypeToStr(specifier.valueType),
                    specifier.formatType,
                    specifier.minWidth,
                    specifier.minDecimal,
                    specifier.flags);
            }
        }
        else
        {
            // Begin formatting specifier.
            if (c == '{' || c == '}')
            {
                if (i+1 < fmt.len && fmt.data[i+1] == c)
                {
                    FMT_TRY_PUSH(c);
                    i++;
                }
                else if (c == '{')
                {
                    inSpecifier = TRUE;
                    specifierStart = i;
                }
            }
            else
            {
                FMT_TRY_PUSH(c);
            }
        }
    }

    return pos;
}

extern String8 Str8_formatv(ARENA* arena, String8 fmt, va_list args)
{
    va_list args2;
    va_copy(args2, args);

    u64 trySize = fmt.len * 2;

    u8* outData = ArenaPushArrayNZ(arena, u8, trySize);
    u64 requiredSize = Str8_formatv_impl(fmt, args, outData, trySize);

    if (requiredSize < trySize)
    {
        ArenaPop(arena, trySize - requiredSize);
    }
    else if (requiredSize > trySize)
    {
        ArenaPop(arena, trySize);
        outData = ArenaPushArrayNZ(arena, u8, requiredSize);

        requiredSize = Str8_formatv_impl(fmt, args2, outData, requiredSize);
    }

    va_end(args2);

    return CLITERAL(String8)
    {
        requiredSize,
        outData,
    };
}

extern String8 Str8_format(ARENA* arena, String8 fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    String8 out = Str8_formatv(arena, fmt, args);

    va_end(args);

    return out;
}


extern String8 Str8_formatv_cstr(ARENA* arena, const char* fmt, va_list args)
{
    return Str8_formatv(arena, Str8_fromCstr((u8 *) fmt), args);
}
extern String8 Str8_format_cstr(ARENA* arena, const char* fmt, ...)
{
    va_list args;

    va_start(args, fmt);

    String8 out = Str8_formatv(arena, Str8_fromCstr((u8 *) fmt), args);

    va_end(args);

    return out;
}
