#ifndef CIRCULARIS_H_
#define CIRCULARIS_H_ 1

#include <stddef.h>
#include "base/types.h"
#include "base/utils.h"

/*
   Token lexer.
*/

#define TOKEN_STR_MAX 256

#define SYMBOL_STR_MAX 32
#define SYMBOL_TABLE_ALIGN 16


/*
   Circularis basic specification:
   Three data types: int, float, string.

   Unary operators: !, ~
   Binary boolean operators: ==, >, >=, <, <=, !=
   Binary bitwise operators: &, ^, |, >>, <<
*/

typedef enum TokenKind {
    TOKEN_NONE = 0,
    TOKEN_EOF,


    TOKEN_LPAREN,
    TOKEN_RPAREN,

    TOKEN_LBRACE,
    TOKEN_RBRACE,

    TOKEN_LBRACKET,
    TOKEN_RBRACKET,

    TOKEN_EQUAL,

    // Token keywords.
    TOKEN_KW_VOID,
    TOKEN_KW_INT,
    TOKEN_KW_FLOAT,
    TOKEN_KW_STRING,

    TOKEN_KW_RETURN,
    TOKEN_KW_IF,

    TOKEN_STR_LIT,
    TOKEN_CHR_LIT,
    TOKEN_INT_LIT,
    TOKEN_FLOAT_LIT,

    TOKEN_IDENT,

    TOKEN_COMMA,
    TOKEN_SEMICOLON,

    TOKEN_OP_ADD,
    TOKEN_OP_SUB,
    TOKEN_OP_MUL,
    TOKEN_OP_DIV,
    TOKEN_OP_MOD,

    TOKEN_OP_NOT,
    TOKEN_OP_BW_NEG,
    TOKEN_OP_EQ,

    TOKEN_OP_GTR,
    TOKEN_OP_GEQ,
    TOKEN_OP_NEQ,
    TOKEN_OP_LES,
    TOKEN_OP_LEQ,

    TOKEN_BW_XOR,
    TOKEN_BW_OR,
    TOKEN_BW_AND,
} TokenKind;

typedef struct FileLocation {
    const char* fileName;
    int line, column;
} FileLocation;

typedef struct Token {
    TokenKind type;
    Token* next;
    FileLocation location;

    char s[TOKEN_STR_MAX+1];
    int sLen;


    Token() : next(NULL) {}
    Token(TokenKind type, char c, int len) : type(type), sLen(len), next(NULL)
    {
        s[0] = c;
    }
} Token;

class TokenScanner {
public:
    const char* baseSource;
    const char* cursor;

    const char* sourceFileName;

    int sourceLen;
    int line;
    int column;

    // Token list.
    Token* tokens;
    int tokenCount;
    int tokenCapacity;

    TokenScanner();
    ~TokenScanner();

    static void init(TokenScanner* self, const char* srcCode, int srcLen);

    static void reserve(TokenScanner* self, int desired);
    static Token* pushToken(TokenScanner* self, Token token);

    static Token retrieveNextToken(TokenScanner* self);
    static void getTokens(TokenScanner* self);

    static void backtrack(TokenScanner* self, int by);
    static const char* peek(TokenScanner* self, int offset);


    static void print(TokenScanner* self);
private:
    static constexpr size_t TOKEN_SCANNER_CAPACITY = 256;
};


extern int IsSpace(char ch);
extern int IsEOF(char ch);
extern int IsAlphabetical(char ch);
extern int IsAlphaNumerical(char ch);
extern int IsOperatorSymbol(char ch);

extern int CalcPseudoHashBounded(const char* str, int strLen, int arraySize);
extern int CstrLen(const char* str);

extern const char* TokenKindToStr(TokenKind kind);

#define PrintError(fmt, ...) fprintf(stderr, LOG_FG_BRIGHT_BLUE "%s: " LOG_COLOR_RESET fmt, __func__, ##__VA_ARGS__)
#define TOKEN_FMT(token) (int) (token)->sLen, (const char *) (token)->s



/*
   Symbol table.
*/

typedef struct Symbol {
    int32_t tokenType;
    int flags;
    int len;
    char val[SYMBOL_STR_MAX+1];
} Symbol;

struct SymbolTableBucket {
    struct SymbolTableBucket* next;
    Symbol symbol;
    int occupied;
};

struct SymbolTable {
    // It's gonna be a hash table.
    SymbolTableBucket* buckets;
    int bucketCount;
    int capacity;

    int initialized;

    SymbolTable();
    ~SymbolTable();

    static void populate(SymbolTable* self, const char* syms[], const TokenKind types[], const int symCount);
    static SymbolTableBucket* insert(SymbolTable* self, const char* str);
    static b32 hasShallow(SymbolTable* self, const char* str, int* hash);
    static SymbolTableBucket* get(SymbolTable* self, const char* str);

    static void print(SymbolTable* self);
    //static b32 has(SymbolTable* self, const char* str, int* hash);
};

struct FunctionDeclaration {
    SymbolTable parameters;
    SymbolTable localVariables;
    SymbolTable functionCalls;
};

/*
   Token parser.
*/

typedef enum AST_types {
    AST_NONE = 0,

    AST_ROOT,

    AST_TYPE,
    AST_IDENT,

    AST_FACTOR,

    AST_OP_ADD,
    AST_OP_SUB,
    AST_OP_MUL,
    AST_OP_DIV,
    AST_OP_MOD,

    AST_FUNCTION_RET_TYPE,
    AST_FUNCTION_IDENT,
    AST_FUNCTION_ARG,
    AST_FUNCTION_DECL,
    AST_FUNCTION_DEF,
    AST_BLOCK,

    AST_GLOBAL_DECL,
    AST_GLOBAL_DEF,

    AST_RETURN_STMT,

    AST_EXPR,
} AST_types;

typedef enum AST_valueTypes {
    AST_INT,
    AST_FLOAT,
    AST_STRING,
} AST_valueTypes;

typedef struct AST_value {
    AST_valueTypes type;
    union {
        i32 i;
        f32 f;
        const char* str;
    };
    int sLen;
} AST_value;

typedef struct AST_Node {
    AST_types type;

    AST_value value;

    AST_Node* parent;

    // Direct pointers to nodes.
    AST_Node* A;
    AST_Node* B;
    AST_Node* C;
    AST_Node* D;

    // Array of auxiliary nodes.
    AST_Node** nodes;
    int nodeCount;

    AST_Node();
    ~AST_Node();

    static void init(AST_Node* self);
    static void terminate(AST_Node* self);

    static void printValue(AST_Node* self);
    static void print(AST_Node* self, int level, const char* label, ...);
} AST_Node;


extern const char* AstTypeToString(AST_types type);

/*
   Recursive descent parser.
*/

/*
   Grammar of the language:
   start -> (globalDeclaration|globalDefinition|functionDeclaration|functionDefinition)
   functionDeclaration -> TYPE name "(" (varDecl ",")* ")"
   functionDefinition -> TYPE name "(" (varDecl ",")* ")" body

   varDecl -> ("int"|"float"|"string") NAME
   name -> NAME
   body -> "{" expr "}"
*/

// TODO: Fix some memory leaks when
// the nodes are _FREE(node).

class Parser {
public:
    AST_Node* root;
    Token* tokens;
    Token* cursor;

    Parser();
    ~Parser();

    static void init(Parser* self, Token* tokens);

    static int parseStart(Parser* self);
    static AST_Node* functionDeclaration(Parser* self, AST_Node* parent);
    static AST_Node* functionDefinition(Parser* self, AST_Node* parent);

    static AST_Node* block(Parser* self, AST_Node* parent);
    static AST_Node* expr(Parser* self, AST_Node* parent);

    static AST_Node* returnStmt(Parser* self, AST_Node* parent);

    static AST_Node* factor(Parser* self, AST_Node* parent);
    static AST_Node* term(Parser* self, AST_Node* parent);

    static AST_Node* type(Parser* self, AST_Node* parent, b32 isFunction);
    static AST_Node* functionArg(Parser* self, AST_Node* parent);

    static AST_Node* globalDecl(Parser* self, AST_Node* parent);
    static AST_Node* globalDef(Parser* self, AST_Node* parent);


    static int name(Parser* self, AST_Node* parent);

    static int consume(Parser* self, const char* str);
    static int consumeOptional(Parser* self, const char* str);
    static int consumeGo(Parser* self);

    static int peek(Parser* self, const char* str, int offsetAhead);
    static int peekType(Parser* self, TokenKind kind, int offsetAhead);

    static void backtrack(Parser* self, int offset);
};

/*
   Compiler.
*/

class Compiler {
public:
    const char* source;
    TokenScanner scanner;
    Parser parser;


    SymbolTable keywords;
    SymbolTable symbols;

    SymbolTable globals;

    FunctionDeclaration functionDeclarations[16];
    int functionDeclCount;

    Compiler();
    ~Compiler();

    static void init(Compiler* self, const char* sourceCode);

    static void doLex(Compiler* self);
    static void doParse(Compiler* self);

    static void printInfo(Compiler* self);
};

extern Compiler* gCompiler;


#endif /* CIRCULARIS_H_ */
