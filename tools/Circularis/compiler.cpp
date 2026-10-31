#include "circularis.h"
#include "base/types.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

/*
   Symbol table.
*/

static const char* symbolKeywords[] = {
    "void",
    "int",
    "float",
    "string",
    "return",
    "if",
};
static TokenKind keywordKinds[] = {
    TOKEN_KW_VOID,
    TOKEN_KW_INT,
    TOKEN_KW_FLOAT,
    TOKEN_KW_STRING,
    TOKEN_KW_RETURN,
    TOKEN_KW_IF,
};
#define SYMBOL_KEYWORD_COUNT 6


static const char* operatorSymbols[] = {
    "==", ">=", "<=", "!=",
    "+", "-", "*", "/", "%",
    "=", "!", "~", ">", "<",
    "(", ")", "{", "}", "[", "]",
    ",", ";",
};
static TokenKind operatorKinds[] = {
    TOKEN_OP_EQ,
    TOKEN_OP_GEQ,
    TOKEN_OP_LEQ,
    TOKEN_OP_NEQ,

    TOKEN_OP_ADD, TOKEN_OP_SUB, TOKEN_OP_MUL,
    TOKEN_OP_DIV, TOKEN_OP_MOD,

    TOKEN_EQUAL,
    TOKEN_OP_NOT,
    TOKEN_OP_BW_NEG,
    TOKEN_OP_GTR,
    TOKEN_OP_LES,

    TOKEN_LPAREN, TOKEN_RPAREN,
    TOKEN_LBRACE, TOKEN_RBRACE,
    TOKEN_LBRACKET, TOKEN_RBRACKET,

    TOKEN_COMMA,
    TOKEN_SEMICOLON,
};
#define OPERATOR_SYMBOL_COUNT STATIC_ARR_LEN(operatorKinds)



SymbolTable::SymbolTable()
{
    _MEMSET(this, 0, sizeof(SymbolTable));
}

SymbolTable::~SymbolTable()
{
    if (buckets != NULL)
    {
        // Iterate over the buckets first.
        for (int i = 0; i < capacity; ++i)
        {
            SymbolTableBucket* bucket = &buckets[i];
            for (SymbolTableBucket* ptr = bucket->next; ptr != NULL; ptr = ptr->next)
            {
                _FREE(ptr);
            }
        }

        _FREE(buckets);
        buckets = NULL;
    }
}


void SymbolTable::populate(SymbolTable *self,
    const char *syms[], const TokenKind types[], const int symCount)
{
    assert(!self->initialized);

    self->capacity = ((symCount + SYMBOL_TABLE_ALIGN) & ~(SYMBOL_TABLE_ALIGN - 1));
    size_t bytecount = sizeof(SymbolTableBucket) * self->capacity;
    self->buckets = (SymbolTableBucket *) _MALLOC(bytecount);
    _MEMSET(self->buckets, 0, bytecount);

    for (int i = 0; i < symCount; ++i)
    {
        SymbolTableBucket* bucket = self->insert(self, syms[i]);
        if (bucket != NULL)
        {
            bucket->symbol.tokenType = static_cast<int32_t> (types[i]);
        }
    }

    self->initialized = TRUE;
}


SymbolTableBucket* SymbolTable::insert(SymbolTable *self, const char *str)
{
    int len = CstrLen(str);
    int hash = CalcPseudoHashBounded(str, len, self->capacity);

    SymbolTableBucket* bucket = &self->buckets[hash];
    if (bucket->occupied == FALSE)
    {
        // Insert symbol.
        if (len > SYMBOL_STR_MAX)
        {
            len = SYMBOL_STR_MAX;
        }
        _MEMCPY(bucket->symbol.val, str, len);
        bucket->symbol.val[len] = '\0';
        bucket->symbol.len = len;

        bucket->occupied = TRUE;
        self->bucketCount++;

        return bucket;
    }
    else
    {
        // Collision case.
        SymbolTableBucket* newBucket = (SymbolTableBucket *) _MALLOC(sizeof(SymbolTableBucket));
        assert(newBucket != NULL);

        if (len > SYMBOL_STR_MAX)
        {
            len = SYMBOL_STR_MAX;
        }

        _MEMCPY(newBucket->symbol.val, str, len);
        newBucket->symbol.val[len] = '\0';
        newBucket->symbol.len = len;
        newBucket->occupied = TRUE;

        bucket->next = newBucket;
        newBucket->next = NULL;

        return newBucket;
    }

    return NULL;
}


b32 SymbolTable::hasShallow(SymbolTable* self, const char* str, int* hash)
{
    int len = CstrLen(str);
    int h = CalcPseudoHashBounded(str, len, self->capacity);

    return (self->buckets[h].occupied);
}

SymbolTableBucket* SymbolTable::get(SymbolTable *self, const char *str)
{
    int len = CstrLen(str);
    int h = CalcPseudoHashBounded(str, len, self->capacity);

    SymbolTableBucket* bucket = &self->buckets[h];
    for (SymbolTableBucket* ptr = bucket; ptr != NULL; ptr = ptr->next)
    {
        if (ptr->symbol.len == len &&
            (memcmp(ptr->symbol.val, str, ptr->symbol.len) == 0))
        {
            return ptr;
        }
    }

    return NULL;
}



// Open addressing.
/*
b32 SymbolTable::has(SymbolTable *self, const char *str, int *hash)
{
    int len = CstrLen(str);
    int h = CalcPseudoHashBounded(str, len, self->capacity);
    int startH = h;

    while (self->buckets[h].occupied)
    {
        if (memcmp(self->buckets[h].symbol.val, str, len))
        {
            if (hash) *hash = h;
            return TRUE;
        }

        h = (h+1) % self->capacity;

        if (h == startH)
        {
            break;
        }
    }

    if (hash) *hash = h;

    return FALSE;
}
*/

void SymbolTable::print(SymbolTable* self)
{
    for (int i = 0; i < self->capacity; ++i)
    {
        SymbolTableBucket* bucket = &self->buckets[i];
        // Iterate over bucket elements.
        if (bucket->occupied)
        {
            for (SymbolTableBucket* ptr = bucket; ptr != NULL; ptr = ptr->next)
            {
                const char* kindName = TokenKindToStr(static_cast<TokenKind>(ptr->symbol.tokenType));
                fprintf(stderr, "[%s] -> [%.*s]  ", kindName, ptr->symbol.len, ptr->symbol.val);
            }

            fprintf(stderr, "\n");
        }
        else
        {
            fprintf(stderr, "[]\n");
        }
    }
}

        // Buckets.

/*
   Compiler.
*/

Compiler::Compiler()
{
    _MEMSET(this, 0, sizeof(Compiler));
}

Compiler::~Compiler()
{
}


void Compiler::init(Compiler *self, const char *sourceCode)
{
    self->source = sourceCode;

    // Populate symbol tables.
    self->keywords.populate(&self->keywords, symbolKeywords, keywordKinds, SYMBOL_KEYWORD_COUNT);
    self->symbols.populate(&self->symbols, operatorSymbols, operatorKinds, OPERATOR_SYMBOL_COUNT);

    self->scanner.init(&self->scanner, sourceCode, CstrLen(sourceCode));
    self->parser.init(&self->parser, self->scanner.tokens);
}

void Compiler::doLex(Compiler *self)
{
    self->scanner.getTokens(&self->scanner);
}

void Compiler::doParse(Compiler* self)
{
    self->scanner.print(&self->scanner);
    self->parser.parseStart(&self->parser);
}

void Compiler::printInfo(Compiler *self)
{
    // Compiler frontend pipeline.
    fprintf(stderr, "KEYWORD TABLE:\n");
    self->keywords.print(&self->keywords);

    fprintf(stderr, "SYMBOL TABLE:\n");
    self->symbols.print(&self->symbols);


    fprintf(stderr, "FOR PROGRAM:\n");
    fprintf(stderr, "%s\n", self->source);

    fprintf(stderr, "========================\n");
    fprintf(stderr, "\nLEXER OUTPUT:\n");
    self->scanner.print(&self->scanner);

    fprintf(stderr, "========================\n");
    fprintf(stderr, "\nPARSER OUTPUT:\n");
    //self->parser.root->print(self->parser.root, 0);
}
