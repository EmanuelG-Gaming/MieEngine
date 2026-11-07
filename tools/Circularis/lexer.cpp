#include "circularis.h"
#include "base/types.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>


extern int IsSpace(char ch)
{
    return (ch == '\n') ||
           (ch == '\r') ||
           (ch == '\t') ||
           (ch == '\v') ||
           (ch == '\f') ||
           (ch == ' ');
}

extern int IsEOF(char ch)
{
    return (ch == '\0');
}

extern int IsAlphabetical(char ch)
{
    return (ch >= 'a' && ch <= 'z') ||
           (ch >= 'A' && ch <= 'Z');
}

extern int IsAlphaNumerical(char ch)
{
    return (ch >= 'a' && ch <= 'z') ||
           (ch >= 'A' && ch <= 'Z') ||
           (ch >= '0' && ch <= '9');
}

extern int IsDigit(char ch)
{
    return (ch >= '0' && ch <= '9');
}

extern int IsOperatorSymbol(char ch)
{
    return
    (ch == '=') || (ch == '!') ||
    (ch == '>') || (ch == '<') ||
    (ch == '&') || (ch == '^') ||
    (ch == '|') || (ch == '+') || (ch == '-') || (ch == '*') || (ch == '/') || (ch == '%');//||
    //(ch == '(') || (ch == ')') ||
    //(ch == '{') || (ch == '}') ||
    //(ch == '[') || (ch == ']');
}



extern int CstrLen(const char *str)
{
    const char* ptr = str;
    for (; *ptr; ++ptr) {}
    return (int) (ptr - str);
}

extern int CalcPseudoHashBounded(const char *str, int strLen, int arraySize)
{
    // Simple checksum.
    int sum = 0;
    for (int i = 0; i < strLen; ++i)
    {
        sum += str[i];
    }
    sum %= arraySize;

    return sum;
}


extern const char* TokenKindToStr(TokenKind kind)
{
    switch (kind)
    {
        case TOKEN_NONE: return "TOKEN_NONE";
        case TOKEN_EOF: return "TOKEN_EOF";

        case TOKEN_LPAREN: return "TOKEN_LPAREN";
        case TOKEN_RPAREN: return "TOKEN_RPAREN";

        case TOKEN_LBRACE: return "TOKEN_LBRACE";
        case TOKEN_RBRACE: return "TOKEN_RBRACE";

        case TOKEN_LBRACKET: return "TOKEN_LBRACKET";
        case TOKEN_RBRACKET: return "TOKEN_RBRACKET";

        case TOKEN_EQUAL: return "TOKEN_EQUAL";

        case TOKEN_KW_VOID: return "TOKEN_KW_VOID";
        case TOKEN_KW_INT: return "TOKEN_KW_INT";
        case TOKEN_KW_FLOAT: return "TOKEN_KW_FLOAT";
        case TOKEN_KW_STRING: return "TOKEN_KW_STRING";
        case TOKEN_KW_RETURN: return "TOKEN_KW_RETURN";

        case TOKEN_KW_IF: return "TOKEN_KW_IF";
        case TOKEN_KW_ELSE: return "TOKEN_KW_ELSE";
        case TOKEN_KW_FOR: return "TOKEN_KW_FOR";
        case TOKEN_KW_WHILE: return "TOKEN_KW_WHILE";

        case TOKEN_KW_GLOBAL: return "TOKEN_KW_GLOBAL";


        case TOKEN_STR_LIT: return "TOKEN_STR_LIT";
        case TOKEN_CHR_LIT: return "TOKEN_CHR_LIT";
        case TOKEN_INT_LIT: return "TOKEN_INT_LIT";
        case TOKEN_FLOAT_LIT: return "TOKEN_FLOAT_LIT";

        case TOKEN_IDENT: return "TOKEN_IDENT";

        case TOKEN_COMMA: return "TOKEN_COMMA";
        case TOKEN_SEMICOLON: return "TOKEN_SEMICOLON";

        case TOKEN_OP_ADD: return "TOKEN_ADD";
        case TOKEN_OP_SUB: return "TOKEN_SUB";
        case TOKEN_OP_MUL: return "TOKEN_MUL";
        case TOKEN_OP_DIV: return "TOKEN_DIV";
        case TOKEN_OP_MOD: return "TOKEN_MOD";

        case TOKEN_OP_NOT: return "TOKEN_OP_NOT";
        case TOKEN_OP_BW_NEG: return "TOKEN_OP_BW_NEG";
        case TOKEN_OP_EQ: return "TOKEN_OP_EQ";
        case TOKEN_OP_GTR: return "TOKEN_OP_GTR";
        case TOKEN_OP_GEQ: return "TOKEN_OP_GEQ";
        case TOKEN_OP_NEQ: return "TOKEN_OP_NEQ";
        case TOKEN_OP_LES: return "TOKEN_OP_LES";
        case TOKEN_OP_LEQ: return "TOKEN_OP_LEQ";

        case TOKEN_OP_BW_XOR: return "TOKEN_OP_BW_XOR";
        case TOKEN_OP_BW_OR: return "TOKEN_OP_BW_OR";
        case TOKEN_OP_BW_AND: return "TOKEN_OP_BW_AND";
        case TOKEN_OP_BW_SHL: return "TOKEN_OP_BW_SHL";
        case TOKEN_OP_BW_SHR: return "TOKEN_OP_BW_SHR";


        default: return "TOKEN_UNKNOWN";
    }
}


TokenScanner::TokenScanner()
{
    _MEMSET(this, 0, sizeof(TokenScanner));
}


TokenScanner::~TokenScanner()
{
    if (tokens != NULL)
    {
        _FREE(tokens);
        tokens = NULL;
    }
}

void TokenScanner::init(TokenScanner *self, const char *srcCode, int srcLen)
{
    self->baseSource = srcCode;
    self->sourceLen = srcLen;
    self->sourceEnd = self->baseSource + self->sourceLen;

    self->cursor = srcCode;

    self->sourceFileName = "<PLACEHOLDER>";

    // Init token.
    self->tokenCapacity = TOKEN_SCANNER_CAPACITY;
    self->tokenCount = 0;

    size_t bytecount = sizeof(Token) * self->tokenCapacity;
    self->tokens = (Token *) _MALLOC(bytecount);
}



Token TokenScanner::retrieveNextToken(TokenScanner* self)
{
    while (1)
    {
        if (peek(self, 0) == '\0')
        {
            return CLITERAL(Token) { TOKEN_EOF, '\0', 0 };
        }

        // Comments.
        if (peek(self, 0) == '/')
        {
            if (peek(self, 1) == '/')
            {
                // Read single-line comment.
                while (peek(self, 0) != '\n' && peek(self, 0) != '\0')
                {
                    self->cursor++;
                }

                if (peek(self, 0) == '\n')
                {
                    self->line++;
                    self->column = 0;
                    self->cursor++;
                }
            } else if (peek(self, 1) == '*')
            {
                // Read multi-line comment.
                self->cursor += 2;
                while (!(peek(self, 0) == '*' && peek(self, 1) == '/') && peek(self, 0) != '\0')
                {
                    if (peek(self, 0) == '\n')
                    {
                        self->line++;
                        self->column = 0;
                    }
                    self->cursor++;
                }

                if (peek(self, 0) == '*' && peek(self, 1) == '/')
                {
                    self->cursor += 2;
                }
            }

            continue;
        }

        // Spaces.
        if (IsSpace(peek(self, 0)))
        {
            // We encounter a line.
            if (peek(self, 0) == '\n')
            {
                self->line++;
                self->column = 0;
            }
            // No tab support? (4 columns).
            else self->column++;

            self->cursor++;
            continue;
        }

        break;
    }

    if (peek(self, 0) == '"')
    {
        // String literals.
        self->cursor++;
        const char* start = self->cursor;
        while (peek(self, 0) != '"' && !IsEOF(peek(self, 0)))
        {
            self->cursor++;
        }
        size_t length = self->cursor - start;
        if (length > TOKEN_STR_MAX)
        {
            // Clamp length.
            length = TOKEN_STR_MAX;
        }

        Token t;
        t.type = TOKEN_STR_LIT;
        _MEMCPY(t.s, start, length);
        t.s[length] = '\0';
        t.sLen = length;
        t.location.line = self->line;
        t.location.column = self->column;

        // Skip closing quote.
        self->cursor++;

        return t;
    }

    if (peek(self, 0) == '\'')
    {
        // Char literals.
        self->cursor++;
        if (!IsEOF(*self->cursor))
        {
            if (*self->cursor != '\'')
            {
                // Full chr.
                self->cursor++;
            }
            else
            {
                // Empty str.
                Token t;
                t.type = TOKEN_CHR_LIT;
                t.s[0] = '\0';
                t.sLen = 1;
                t.location.line = self->line;
                t.location.column = self->column;

                self->cursor++;

                return t;
            }
        }

        Token t;
        t.type = TOKEN_CHR_LIT;
        t.s[0] = *self->cursor;
        t.s[1] = '\0';
        t.sLen = 1;

        t.location.line = self->line;
        t.location.column = self->column;

        self->cursor++;

        return t;
    }

    // Handle alphabets.
    if (IsAlphabetical(peek(self, 0)) || peek(self, 0) == '_')
    {
        // Handle identifiers and keywords.
        const char* start = self->cursor;
        while (IsAlphaNumerical(peek(self, 0)) || peek(self, 0) == '_')
        {
            self->cursor++;
        }

        size_t length = self->cursor - start;
        if (length > TOKEN_STR_MAX)
        {
            length = TOKEN_STR_MAX;
        }

        Token t;
        t.type = TOKEN_EOF;
        _MEMCPY(t.s, start, length);
        t.s[length] = '\0';
        t.sLen = length;
        t.location.line = self->line;
        t.location.column = self->column;

        // Check if it's in a keyword table.
        SymbolTableBucket* bucket = gCompiler->keywords.get(&gCompiler->keywords, t.s);
        if (bucket != NULL)
        {
            // It's a keyword.
            t.type = static_cast <TokenKind> (bucket->symbol.tokenType);
            return t;
        }
        else
        {
            // We have an identifier.
            t.type = TOKEN_IDENT;
            return t;
        }
    }

    // Handle numbers.
    int floatCounter = 0;
    b32 negative = FALSE;
    if (*self->cursor == '-') negative = TRUE;
    if (IsDigit(peek(self, negative)) || (peek(self, negative) == '.'))
    {
        const char* start = self->cursor;
        if (peek(self, 0) == '-') self->cursor++;

        while (IsAlphaNumerical(peek(self, 0)) || peek(self, 0) == '.')
        {
            if (peek(self, 0) == '.') floatCounter++;
            self->cursor++;
        }
        if (floatCounter > 1)
        {
            return CLITERAL(Token) { TOKEN_EOF, '\0', 0 };
        }

        // "Consume" a character.
        const char* prevCursor = self->cursor;
        if (*self->cursor == 'f')
        {
            if (floatCounter == 1) self->cursor++;
            else return CLITERAL(Token) { TOKEN_EOF, '\0', 0 };
        }

        size_t length = prevCursor - start;
        if (length > TOKEN_STR_MAX)
        {
            length = TOKEN_STR_MAX;
        }

        Token t;
        t.type = (floatCounter == 1) ? TOKEN_FLOAT_LIT : TOKEN_INT_LIT;
        _MEMCPY(t.s, start, length);
        t.s[length] = '\0';
        t.sLen = length;
        t.location.line = self->line;
        t.location.column = self->column;

        return t;
    }

    // Handle operators/symbols.
    if (IsOperatorSymbol(peek(self, 0)))
    {
        const char* start = self->cursor;
        while (IsOperatorSymbol(peek(self, 0)))
        {
            self->cursor++;
        }
        size_t length = self->cursor - start;
        if (length > TOKEN_STR_MAX)
        {
            length = TOKEN_STR_MAX;
        }

        Token t;
        t.type = TOKEN_EOF;
        _MEMCPY(t.s, start, length);
        t.s[length] = '\0';
        t.sLen = length;
        t.location.line = self->line;
        t.location.column = self->column;


        // Check if it's in a keyword table.
        SymbolTableBucket* bucket = gCompiler->symbols.get(&gCompiler->symbols, t.s);
        if (bucket != NULL)
        {
            // It's an operator.
            t.type = static_cast <TokenKind> (bucket->symbol.tokenType);
            return t;
        }
        else
        {
            return t;
        }
    }


    // Trivial to parse tokens.
    // TODO: add support for tokens with multiple lengths through
    // peeking.
    switch (peek(self, 0))
    {
        case '|': self->cursor++; return CLITERAL(Token) { TOKEN_OP_BW_OR, '|', 1 };
        case '&': self->cursor++; return CLITERAL(Token) { TOKEN_OP_BW_AND, '&', 1 };
        case '^': self->cursor++; return CLITERAL(Token) { TOKEN_OP_BW_XOR, '^', 1 };

        case '(': self->cursor++; return CLITERAL(Token) { TOKEN_LPAREN, '(', 1 };
        case ')': self->cursor++; return CLITERAL(Token) { TOKEN_RPAREN, ')', 1 };
        case '{': self->cursor++; return CLITERAL(Token) { TOKEN_LBRACE, '{', 1 };
        case '}': self->cursor++; return CLITERAL(Token) { TOKEN_RBRACE, '}', 1 };
        case '[': self->cursor++; return CLITERAL(Token) { TOKEN_LBRACKET, '[', 1 };
        case ']': self->cursor++; return CLITERAL(Token) { TOKEN_RBRACKET, ']', 1 };

        case ',': self->cursor++; return CLITERAL(Token) { TOKEN_COMMA, ',', 1 };
        case ';': self->cursor++; return CLITERAL(Token) { TOKEN_SEMICOLON, ';', 1 };

        default: break;
    }


    return CLITERAL(Token) { TOKEN_EOF, '\0', 0 };
}



void TokenScanner::getTokens(TokenScanner *self)
{
    Token* prevToken = NULL;
    Token token;

    do {
        token = self->retrieveNextToken(self);
        Token* tokenPtr = self->pushToken(self, token);

        if (prevToken != NULL)
        {
            prevToken->next = tokenPtr;
        };
        prevToken = tokenPtr;
    } while (token.type != TOKEN_EOF);
}



void TokenScanner::reserve(TokenScanner *self, int desired)
{
    if (self->tokens == NULL)
    {
        // Initialize tokens.
        self->tokenCapacity = TOKEN_SCANNER_CAPACITY;
        self->tokens = (Token *) _MALLOC(sizeof(Token) * self->tokenCapacity);
    }

    if (desired >= self->tokenCapacity)
    {
        int newCap = self->tokenCapacity;
        while (desired >= newCap)
        {
            newCap *= 2;
            //fprintf(stderr, "%s: desired: %d newCap: %d\n", __func__, desired, newCap);
        }

        self->tokenCapacity = newCap;
        self->tokens = (Token *) _REALLOC(self->tokens, sizeof(Token) * self->tokenCapacity);
    }
}


Token* TokenScanner::pushToken(TokenScanner *self, Token token)
{
    reserve(self, self->tokenCount + 1);

    Token* ptr = &self->tokens[self->tokenCount];
    *ptr = token;
    self->tokenCount++;

    return ptr;
}

void TokenScanner::backtrack(TokenScanner *self, int by)
{
    const char* ptr = self->cursor - by;

    if (ptr >= self->baseSource)
    {
        self->cursor = ptr;
    } else self->cursor = self->baseSource;
}

char TokenScanner::peek(TokenScanner *self, int offset)
{
    if (self->cursor + offset >= self->sourceEnd)
    {
        return '\0';
    }

    return (self->cursor + offset)[0];
}


void TokenScanner::print(TokenScanner *self)
{
    for (int i = 0; i < self->tokenCount; ++i)
    {
        Token* token = &self->tokens[i];

        const char* typeStr = TokenKindToStr(token->type);

        fprintf(stderr, "[%s]: [%.*s]\n", typeStr,
            (int) token->sLen, (const char *) token->s);
    }
}
