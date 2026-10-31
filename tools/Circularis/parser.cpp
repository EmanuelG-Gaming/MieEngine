#include "circularis.h"

#include <cstdarg>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>

AST_Node::AST_Node()
{
    _MEMSET(this, 0, sizeof(AST_Node));
}

AST_Node::~AST_Node()
{
    terminate(this);
}

void AST_Node::terminate(AST_Node *self)
{
    if (self->A != NULL)
    {
        self->A->terminate(self->A);
        _FREE(self->A);
        self->A = NULL;
    }
    if (self->B != NULL)
    {
        self->B->terminate(self->B);
        _FREE(self->B);
        self->B = NULL;
    }
    if (self->C != NULL)
    {
        self->C->terminate(self->C);
        _FREE(self->C);
        self->C = NULL;
    }
    if (self->D != NULL)
    {
        self->D->terminate(self->D);
        _FREE(self->D);
        self->D = NULL;
    }

    if (self->nodes != NULL)
    {
        for (int i = 0; i < self->nodeCount; ++i)
        {
            AST_Node** node = &self->nodes[i];
            if (*node != NULL)
            {
                (*node)->terminate(*node);
            }
}

        _FREE(self->nodes);
        self->nodes = NULL;
    }
}

void AST_Node::printValue(AST_Node *self)
{
    switch (self->type)
    {
        case AST_FACTOR:
        case AST_IDENT:
        case AST_FUNCTION_IDENT:
        {
            fprintf(stderr, " -> [");
            switch (self->value.type)
            {
                case AST_INT: fprintf(stderr, "i32(%d)", self->value.i); break;
                case AST_FLOAT: fprintf(stderr, "f32(%f)", self->value.f); break;
                case AST_STRING: fprintf(stderr, "str(%s)", self->value.str); break;
                default: fprintf(stderr, "none");
            }
            fprintf(stderr, "]");
        } break;
        default: break;
    }
}

void AST_Node::print(AST_Node *self, int level, const char* label, ...)
{
    for (int i = 0; i < level; ++i)
    {
        // Indents.
        fprintf(stderr, "%4c", ' ');
    }

    char buf[1024];

    va_list args;
    va_start(args, label);
    int len = vsnprintf(buf, 1024, label, args);
    va_end(args);

    fprintf(stderr, "%.*s-> [%s]", len, buf, AstTypeToString(self->type));
    printValue(self);
    fprintf(stderr, "\n");

    if (self->A != NULL)
    {
        self->A->print(self->A, level+1, "A ");
    }
    if (self->B != NULL)
    {
        self->B->print(self->B, level+1, "B ");
    }
    if (self->C != NULL)
    {
        self->C->print(self->C, level+1, "C ");
    }
    if (self->D != NULL)
    {
        self->D->print(self->D, level+1, "D ");
    }

    if (self->nodes != NULL)
    {
        for (int i = 0; i < level; ++i)
        {
            fprintf(stderr, "%4c", ' ');
        }
        fprintf(stderr, "LIST:\n");

        for (int i = 0; i < self->nodeCount; ++i)
        {
            AST_Node* node = self->nodes[i];
            if (node != NULL)
            {
                node->print(node, level+1, "%d ", i);
            }
        }
    }
}


extern const char* AstTypeToString(AST_types type)
{
    switch (type)
    {
        case AST_NONE: return "AST_NONE";
        case AST_TYPE: return "AST_TYPE";
        case AST_IDENT: return "AST_IDENT";

        case AST_ROOT: return "AST_ROOT";

        case AST_FUNCTION_RET_TYPE: return "AST_FUNCTION_RET_TYPE";
        case AST_FUNCTION_IDENT: return "AST_FUNCTION_IDENT";
        case AST_FUNCTION_ARG: return "AST_FUNCTION_ARG";
        case AST_FUNCTION_DECL: return "AST_FUNCTION_DECL";
        case AST_FUNCTION_DEF: return "AST_FUNCTION_DEF";

        case AST_BLOCK: return "AST_BLOCK";

        case AST_FACTOR: return "AST_FACTOR";

        case AST_OP_ADD: return "AST_OP_ADD";
        case AST_OP_SUB: return "AST_OP_SUB";
        case AST_OP_MUL: return "AST_OP_MUL";
        case AST_OP_DIV: return "AST_OP_DIV";
        case AST_OP_MOD: return "AST_OP_MOD";

        case AST_GLOBAL_DECL: return "AST_GLOBAL_DECL";
        case AST_GLOBAL_DEF: return "AST_GLOBAL_DEF";


        case AST_RETURN_STMT: return "AST_RETURN_STMT";
        case AST_EXPR: return "AST_EXPR";

        default: return "AST_UNKNOWN_TYPE";
    }
}

/*
   Parser.
*/

Parser::Parser()
{
    _MEMSET(this, 0, sizeof(Parser));
}
Parser::~Parser()
{
    if (root)
    {
        root->terminate(root);
    }
}



void Parser::init(Parser *self, Token* tokens)
{
    self->root = (AST_Node *) _MALLOC(sizeof(AST_Node));
    self->tokens = tokens;
    self->cursor = tokens;
}

#define NODE_CAPACITY 64



// Grammar rule:
// type -> ("int"|"float"|"string")
AST_Node* Parser::type(Parser* self, AST_Node *parent, b32 isFunction)
{
    int hasType = 0;
    if (peek(self, "int", 0)) {
        hasType = 1;
    } else if (peek(self, "float", 0)) {
        hasType = 1;
    } else if (peek(self, "string", 0)) {
        hasType = 1;
    }
    if (isFunction && (peek(self, "void", 0) || peek(self, "fn", 0)))
    {
        hasType = 1;
    }

    PrintError("current token %.*s\n", TOKEN_FMT(self->cursor));

    if (!hasType)
    {
        PrintError("Syntax error: type is not valid!\n");
        return NULL;
    }
    consumeGo(self);
    if (peek(self, "int", 0) || peek(self, "float", 0) || peek(self, "string", 0) ||
        peek(self, "void", 0) || peek(self, "fn", 0))
    {
        PrintError("Syntax error: cannot have multiple variable types!\n");
        return NULL;
    }

    // Get the NAME token and put it in the tree.
    AST_Node* a = (AST_Node *) _CALLOC(1, sizeof(AST_Node));
    assert(a != NULL);

    a->type = (isFunction) ? AST_FUNCTION_RET_TYPE : AST_TYPE;
    a->parent = parent;
    a->value.str = self->cursor->s;
    a->value.sLen = self->cursor->sLen;

    //consumeGo(self);
    //consumeOptional(self, ",");

    return a;
}

// Grammar rule:
// globalDecl -> ("global")* TYPE name;
AST_Node* Parser::globalDecl(Parser *self, AST_Node *parent)
{
    consumeOptional(self, "global");

    AST_Node* node = (AST_Node *) _CALLOC(1, sizeof(AST_Node));
    assert(node != NULL);
    node->type = AST_GLOBAL_DECL;
    node->parent = parent;

    AST_Node* a = type(self, node, FALSE);
    if (a == NULL)
    {
        PrintError("Syntax error: Expected type.\n");
        return NULL;
    }
    node->A = a;
    node->A->parent = node;


    Token* ident = self->cursor;
    consumeGo(self);

    AST_Node* b = (AST_Node *) _CALLOC(1, sizeof(AST_Node));
    assert(b != NULL);
    b->type = AST_IDENT;
    b->value.type = AST_STRING;
    b->value.str = ident->s;
    b->value.sLen = ident->sLen;

    node->B = b;
    node->B->parent = node;


    if (!consumeOptional(self, ";"))
    {
        return NULL;
    }

    return node;
}

// Grammar rule:
// globalDef -> ("global")* TYPE name = expr;
AST_Node* Parser::globalDef(Parser *self, AST_Node *parent)
{
    consumeOptional(self, "global");

    AST_Node* node = (AST_Node *) _CALLOC(1, sizeof(AST_Node));
    assert(node != NULL);
    node->type = AST_GLOBAL_DEF;
    node->parent = parent;

    AST_Node* a = type(self, node, FALSE);
    if (a == NULL)
    {
        PrintError("Syntax error: Expected type.\n");
        return NULL;
    }
    node->A = a;
    node->A->parent = node;

    Token* ident = self->cursor;
    consumeGo(self);
    AST_Node* b = (AST_Node *) _CALLOC(1, sizeof(AST_Node));
    assert(b != NULL);
    b->type = AST_IDENT;
    b->value.type = AST_STRING;
    b->value.str = ident->s;
    b->value.sLen = ident->sLen;

    node->B = b;
    node->B->parent = node;


    if (!consume(self, "="))
    {
        PrintError("Syntax error: Expected '='.\n");
        return NULL;
    }

    AST_Node* c = expr(self, node);
    if (c == NULL)
    {
        PrintError("Syntax error: Expected expression.\n");
        return NULL;
    }
    node->C = c;
    node->C->parent = node;

    if (!consumeOptional(self, ";"))
    {
        PrintError("Syntax error: Expected semicolon ;\n");
        return NULL;
    }


    return node;
}

// Grammar rule:
// returnStmt -> "return" expr ";"
AST_Node* Parser::returnStmt(Parser *self, AST_Node *parent)
{
    if (!consume(self, "return"))
    {
        PrintError("Syntax error: expected 'return'\n");
        return NULL;
    }

    AST_Node* a = (AST_Node *) _CALLOC(1, sizeof(AST_Node));
    assert(a != NULL);
    a->type = AST_RETURN_STMT;
    a->parent = parent;

    AST_Node* e = expr(self, a);
    if (e == NULL)
    {
        PrintError("Syntax error: expected expression: %*s\n", TOKEN_FMT(self->cursor));
        return NULL;
    }
    a->A = e;

    if (!consume(self, ";"))
    {
        PrintError("Syntax error: expected ';'.\n");
        return NULL;
    }

    return a;
}

// Either an INT, a FLOAT or an IDENT.
AST_Node* Parser::factor(Parser *self, AST_Node *parent)
{
    if (peekType(self, TOKEN_INT_LIT, 0))
    {
        AST_Node* node = (AST_Node *) _CALLOC(1, sizeof(AST_Node));
        assert(node != NULL);
        node->type = AST_FACTOR;
        node->parent = parent;
        node->value.type = AST_INT;
        node->value.i = atoi(self->cursor->s);
        consumeGo(self);
        return node;
    }
    if (peekType(self, TOKEN_FLOAT_LIT, 0))
    {
        AST_Node* node = (AST_Node *) _CALLOC(1, sizeof(AST_Node));
        assert(node != NULL);
        node->type = AST_FACTOR;
        node->parent = parent;
        node->value.type = AST_FLOAT;
        node->value.f = atof(self->cursor->s);
        consumeGo(self);
        return node;
    }
    if (peekType(self, TOKEN_IDENT, 0))
    {
        AST_Node* node = (AST_Node *) _CALLOC(1, sizeof(AST_Node));
        assert(node != NULL);
        node->type = AST_IDENT;
        node->parent = parent;
        node->value.type = AST_STRING;
        node->value.str = self->cursor->s;
        node->value.sLen = self->cursor->sLen;
        consumeGo(self);
        return node;
    }

    // Verify if there is a parenthesis.
    if (peek(self, "(", 0))
    {
        consume(self, "(");
        AST_Node* node = expr(self, parent);
        consume(self, ")");
        return node;
    }

    PrintError("Syntax error: Expected a number (INT or FLOAT) or an identifier.\n");
    return NULL;
}

// Multiplication and division. (*, /)
AST_Node* Parser::term(Parser *self, AST_Node *parent)
{
    // Obtain the first factor.
    AST_Node* left = factor(self, parent);
    if (left == NULL)
    {
        PrintError("Syntax error: Expected a factor for the LHS.\n");
        return NULL;
    }

    // As long as the next token is * or /, create an operation node.
    while (peek(self, "*", 0) || peek(self, "/", 0))
    {
        Token* opToken = self->cursor;
        consumeGo(self); // self->token.

        //Token* opToken = self->cursor;

        AST_Node* opNode = (AST_Node *) _CALLOC(1, sizeof(AST_Node));
        switch (opToken->type)
        {
            case TOKEN_OP_MUL: opNode->type = AST_OP_MUL; break;
            case TOKEN_OP_DIV: opNode->type = AST_OP_DIV; break;
            default: opNode->type = AST_NONE;
        }
        opNode->parent = parent;

        opNode->A = left;
        left->parent = opNode;

        AST_Node* right = factor(self, opNode);
        if (right == NULL)
        {
            PrintError("Syntax error: Expected a factor for the RHS.\n");
            return NULL;
        }
        opNode->B = right;
        left = opNode;
    }

    return left;
}


// Grammar rule:
// expr -> ("{")* (returnStmt | expr)* ("}")*
// expr -> term + expr
//       | term - expr
//       | term
// term -> factor * term
//       | factor / term
//       | term
// factor -> IDENT | NUMBER

// The expression would be parsed recursively.
// Operator precendence is applied to math expressions, especially the
// multiplication and division.
AST_Node* Parser::expr(Parser* self, AST_Node* parent)
{
    AST_Node* left = term(self, parent);
    if (left == NULL)
    {
        PrintError("Syntax error: Expected a term for the LHS.\n");
        return NULL;
    }

    while (peek(self, "+", 0) || peek(self, "-", 0))
    {
        Token* opToken = self->cursor;
        consumeGo(self);

        AST_Node* opNode = (AST_Node *) _CALLOC(1, sizeof(AST_Node));
        assert(opNode != NULL);
        switch (opToken->type)
        {
            case TOKEN_OP_ADD: opNode->type = AST_OP_ADD; break;
            case TOKEN_OP_SUB: opNode->type = AST_OP_SUB; break;
            default: opNode->type = AST_NONE; break;
        }
        opNode->parent = parent;

        opNode->A = left;
        left->parent = opNode;

        AST_Node* right = term(self, opNode);
        if (right == NULL)
        {
            PrintError("Syntax error: Expected a term for the RHS.\n");
            return NULL;
        }
        opNode->B = right;
        left = opNode;
    }
    return left;


    /*
    AST_Node* node = (AST_Node *) _CALLOC(1, sizeof(AST_Node));
    assert(node != NULL);

    node->type = AST_EXPR;
    node->parent = parent;

    int braceCount = 0;
    while (peek(self, "{", 0))
    {
        consume(self, "{");
        braceCount++;
    }

    AST_Node* currentChild = NULL;
    while (peek(self, "}", 0) == FALSE)
    { // We have to make the statements correctly consume the tokens.
        if (peek(self, "return", 0))
        {
            currentChild = returnStmt(self, node);
        }
        else if (peek(self, "{", 0))
        {
            currentChild = block(self, node);
        }
        else
        {
            PrintError("Syntax error: unexpected token %.*s.\n", TOKEN_FMT(self->cursor));
            goto err;
        }

        if (currentChild == NULL)
        {
            goto err;
        }

        // Add static vector.
        if (node->nodes == NULL)
        {
            size_t bytecount = sizeof(AST_Node *) * NODE_CAPACITY;
            node->nodes = (AST_Node **) _MALLOC(bytecount);
        }
        node->nodes[node->nodeCount] = currentChild;
        node->nodeCount++;
    }

    while (peek(self, "}", 0))
    {
        consume(self, "}");
        braceCount--;
    }
    if (braceCount != 0)
    {
        PrintError("Syntax error: Curly braces not closed correctly!\n");
        goto err;
    }

    return node;
    */

err:
    /*
    if (currentChild != NULL)
    {
        _FREE(currentChild);
        currentChild = NULL;
    }
    if (node != NULL)
    {
        _FREE(node);
        node = NULL;
    }
    */

    return NULL;
}



// Grammar rule:
// block -> "{" (returnStmt)* "}"
AST_Node* Parser::block(Parser* self, AST_Node *parent)
{
    if (!consume(self, "{"))
    {
        PrintError("Syntax error: Expected opening curly brace: '{'\n");
        return NULL;
    }

    AST_Node* node = (AST_Node *) _CALLOC(1, sizeof(AST_Node));
    assert(node != NULL);

    node->type = AST_BLOCK;
    node->parent = parent;

    AST_Node* currentChild = NULL;
    int hasClosing = peek(self, "}", 0);
    while (!hasClosing)
    {
        if (peek(self, "return", 0))
        {
            currentChild = returnStmt(self, node);
        }
        else
        {
            // TODO: Ok.
            currentChild = NULL;
            PrintError("Syntax error: No return statement! Current token: %.*s\n", TOKEN_FMT(self->cursor));
            return NULL;
        }

        if (currentChild == NULL)
        {
            PrintError("Syntax error: Couldn't parse things in function block!\n");
            goto err;
        }

        // Add static vector.
        if (node->nodes == NULL)
        {
            size_t bytecount = sizeof(AST_Node *) * NODE_CAPACITY;
            node->nodes = (AST_Node **) _MALLOC(bytecount);
        }
        node->nodes[node->nodeCount] = currentChild;
        node->nodeCount++;

        hasClosing = peek(self, "}", 0);
    }
    if (!hasClosing)
    {
        PrintError("Syntax error: Couldn't parse function block. Expected closing curly brace.\n");
        goto err;
    }

    if (!consume(self, "}"))
    {
        PrintError("Syntax error: Expected closing curly brace: '}'\n");
        return NULL;
    }

    return node;

err:
    /*
    if (master == NULL)
    {
        return NULL;
    }

    if (master->nodes != NULL)
    {
        for (int i = 0; i < master->nodeCount; ++i)
        {
            _FREE(master->nodes[i]);
        }
        _FREE(master->nodes);
    }
    _FREE(master);
    */

    return NULL;
}

// Grammar rule:
// functionArg -> ("int"|"float"|"string") NAME.
AST_Node* Parser::functionArg(Parser *self, AST_Node* parent)
{
    int hasType = 0;
    if (peek(self, "int", 0)) {
        hasType = 1;
    } else if (peek(self, "float", 0)) {
        hasType = 1;
    } else if (peek(self, "string", 0)) {
        hasType = 1;
    }

    if (!hasType)
    {
        PrintError("Syntax error: function argument has no valid type!\n");
        PrintError("In token: %.*s\n", self->cursor->sLen, self->cursor->s);

        return NULL;
    }
    consumeGo(self);
    if (peek(self, "int", 0) || peek(self, "float", 0) || peek(self, "string", 0))
    {
        PrintError("Syntax error: cannot have multiple variable types!\n");
        return NULL;
    }

    // Get the NAME token and put it in the tree.
    AST_Node* a = (AST_Node *) _CALLOC(1, sizeof(AST_Node));
    assert(a != NULL);

    a->type = AST_FUNCTION_ARG;
    a->parent = parent;
    a->value.str = self->cursor->s;
    a->value.sLen = self->cursor->sLen;

    consumeGo(self);
    consumeOptional(self, ",");

    return a;
}

// Grammar rule:
// functionDeclaration -> type NAME "(" (varDecl ",")* ")"
AST_Node* Parser::functionDeclaration(Parser *self, AST_Node* parent)
{
    /*
    AST_Node* master = (AST_Node *) _CALLOC(1, sizeof(AST_Node));
    assert(master != NULL);

    master->type = AST_FUNCTION_DECL;
    master->parent = parent;
    master->nodeCount = 0;


    AST_Node* a = type(self, master, TRUE);
    master->A = a;

    consumeGo(self);

    // Parse function name.
    AST_Node* b = (AST_Node *) _CALLOC(1, sizeof(AST_Node));
    assert(b != NULL);
    b->type = AST_FUNCTION_NAME;
    b->parent = master;
    b->value.str = self->cursor->s;
    b->value.sLen = self->cursor->sLen;

    master->B = b;

    if (!consume(self, "("))
    {
        //_FREE(master);
        //_FREE(a);
        //_FREE(b);
        return NULL;
    }

    AST_Node* currentArg = NULL;
    int hasClosing = peek(self, ")", 0);
    while (!hasClosing)
    {
        // Parse function arguments.
        PrintError("token: %*s\n", TOKEN_FMT(self->cursor));
        currentArg = functionArg(self, master);
        if (currentArg == NULL)
        {
            PrintError("Syntax error: Couldn't parse function argument!\n");
            goto err;
        }

        // Add static vector.
        if (master->nodes == NULL)
        {
            size_t bytecount = sizeof(AST_Node *) * NODE_CAPACITY;
            master->nodes = (AST_Node **) _MALLOC(bytecount);
        }
        master->nodes[master->nodeCount] = currentArg;
        master->nodeCount++;

        hasClosing = peek(self, ")", 0);
    }
    // Parse function arguments.
    if (!hasClosing)
    {
        PrintError("Syntax error: Couldn't parse function declaration. Expected closing parenthesis.\n");
        goto err;
    }
    if (!consume(self, ")"))
    {
        goto err;
    }
    if (!consume(self, ";"))
    {
        //PrintError("Syntax error: Expected ");
        goto err;
    }


    return master;
    */

    AST_Node* master = (AST_Node *) _CALLOC(1, sizeof(AST_Node));
    assert(master != NULL);

    master->type = AST_FUNCTION_DECL;
    master->parent = parent;
    master->nodeCount = 0;


    AST_Node* a = type(self, master, TRUE);
    assert(a != NULL);
    master->A = a;


    Token* tokenPrev = self->cursor;
    consumeGo(self);

    // Parse function name.
    AST_Node* b = (AST_Node *) _CALLOC(1, sizeof(AST_Node));
    assert(b != NULL);
    b->type = AST_FUNCTION_IDENT;
    b->parent = master;
    b->value.type = AST_STRING;
    b->value.str = tokenPrev->s;
    b->value.sLen = tokenPrev->sLen;

    master->B = b;

    if (!consume(self, "("))
    { 
        return NULL;
    }

    AST_Node* currentArg = NULL;
    int hasClosing = peek(self, ")", 0);
    while (!hasClosing)
    {
        // Parse function arguments.
        //PrintError("token: %*s\n", TOKEN_FMT(self->cursor));

        currentArg = functionArg(self, master);
        if (currentArg == NULL)
        {
            PrintError("Syntax error: Couldn't parse function argument!\n");
            goto err;
        }

        // Add static vector.
        if (master->nodes == NULL)
        {
            size_t bytecount = sizeof(AST_Node *) * NODE_CAPACITY;
            master->nodes = (AST_Node **) _MALLOC(bytecount);
        }
        master->nodes[master->nodeCount] = currentArg;
        master->nodeCount++;

        hasClosing = peek(self, ")", 0);
    }
    // Parsed function arguments.
    if (!hasClosing)
    {
        PrintError("Syntax error: Couldn't parse function declaration. Expected closing parenthesis.\n");
        goto err;
    }
    if (!consume(self, ")"))
    {
        goto err;
    }
    if (!consumeOptional(self, ";"))
    {
        goto err;
    }

    return master;


err:
    /*
    if (master->nodes != NULL)
    {
        for (int i = 0; i < master->nodeCount; ++i)
        {
            _FREE(master->nodes[i]);
        }
        _FREE(master->nodes);
    }

    _FREE(master->A);
    _FREE(master->B);
    _FREE(master);
    */

    return NULL;
}


// We have to keep a distinction between function definitions
// and function declarations,
// so we parse a code block using the required "{" and "}".
// The error label simply handles the memory and returns NULL.
//
// This NULL value will be used to distinguish between
// function/variable declarations and definitions in a language with a C-like syntax,
// since function declarations are done using the semicolon ;
// and definitions simply have the curly braces.

// Grammar rule:
// functionDefinition -> type NAME "(" (varDecl ",")* ")"
AST_Node* Parser::functionDefinition(Parser *self, AST_Node* parent)
{
    AST_Node* master = (AST_Node *) _CALLOC(1, sizeof(AST_Node));
    assert(master != NULL);

    master->type = AST_FUNCTION_DEF;
    master->parent = parent;
    master->nodeCount = 0;


    AST_Node* a = type(self, master, TRUE);
    assert(a != NULL);
    master->A = a;

    Token* prevToken = self->cursor;
    consumeGo(self);

    // Parse function name.
    AST_Node* b = (AST_Node *) _CALLOC(1, sizeof(AST_Node));
    assert(b != NULL);
    b->type = AST_FUNCTION_IDENT;
    b->parent = master;
    b->value.type = AST_STRING;
    b->value.str = prevToken->s;
    b->value.sLen = prevToken->sLen;

    master->B = b;

    if (!consume(self, "("))
    {
        /*
        _FREE(master);
        _FREE(a);
        _FREE(b);
        */
        return NULL;
    }

    AST_Node* c = NULL;


    AST_Node* currentArg = NULL;
    int hasClosing = peek(self, ")", 0);
    while (!hasClosing)
    {
        // Parse function arguments.
        PrintError("token: %*s\n", TOKEN_FMT(self->cursor));

        currentArg = functionArg(self, master);
        if (currentArg == NULL)
        {
            PrintError("Syntax error: Couldn't parse function argument!\n");
            goto err;
        }

        // Add static vector.
        if (master->nodes == NULL)
        {
            size_t bytecount = sizeof(AST_Node *) * NODE_CAPACITY;
            master->nodes = (AST_Node **) _MALLOC(bytecount);
        }
        master->nodes[master->nodeCount] = currentArg;
        master->nodeCount++;

        hasClosing = peek(self, ")", 0);
    }
    // Parsed function arguments.
    if (!hasClosing)
    {
        PrintError("Syntax error: Couldn't parse function definition. Expected closing parenthesis.\n");
        goto err;
    }
    if (!consume(self, ")"))
    {
        goto err;
    }

    c = block(self, master);
    if (c == NULL)
    {
        goto err;
    }
    master->C = c;

    return master;

err:
    if (master == NULL)
    {
        return NULL;
    }
    /*
    if (master->nodes != NULL)
    {
        for (int i = 0; i < master->nodeCount; ++i)
        {
            _FREE(master->nodes[i]);
        }
        _FREE(master->nodes);
    }

    if (master->A)
    {
        _FREE(master->A);
        master->A = NULL;
    }
    if (master->B)
    {
        _FREE(master->B);
        master->B = NULL;
    }
    if (master->C)
    {
        _FREE(master->C);
        master->C = NULL;
    }
    _FREE(master);
    */

    return NULL;
}

// Grammar rule:
// start -> (globaDecl|globalDef|functionDeclaration|functionDefinition)*
int Parser::parseStart(Parser *self)
{
    AST_Node* root = (AST_Node *) _CALLOC(1, sizeof(AST_Node));
    assert(root != NULL);
    root->type = AST_ROOT;

    AST_Node* currentNode = NULL;
    while (self->cursor->type != TOKEN_EOF)
    {
        // Join the cursors (backtracking).
        Token* prevCursor = self->cursor;
        currentNode = globalDecl(self, root);
        if (currentNode == NULL)
        {
            self->cursor = prevCursor;
            currentNode = globalDef(self, root);
        }
        if (currentNode == NULL)
        {
            self->cursor = prevCursor;
            currentNode = functionDeclaration(self, root);
        }
        if (currentNode == NULL)
        {
            // Fallback: try function definitions.
            self->cursor = prevCursor;
            currentNode = functionDefinition(self, root);
        }

        if (currentNode == NULL)
        {
            PrintError("Syntax error: !!! Incorrect declaration or definition!\n");
            goto err;
            return -1;
        }

        // Add static vector.
        if (root->nodes == NULL)
        {
            size_t bytecount = sizeof(AST_Node *) * NODE_CAPACITY;
            root->nodes = (AST_Node **) _MALLOC(bytecount);
        }
        root->nodes[root->nodeCount] = currentNode;
        root->nodeCount++;
    }

    fprintf(stderr, "PRINT RESULT:\n");
    root->print(root, 0, "");

    if (self->cursor->type != TOKEN_EOF)
    {
        PrintError("Syntax error: Parser didn't reach the end (EOF)!\n");
        goto err;
    }

    self->root = root;

    return 0;

err:
    /*
    if (root->nodes != NULL)
    {
        for (int i = 0; i < root->nodeCount; ++i)
        {
            if (root->nodes[i] != NULL)
            {
                _FREE(root->nodes[i]);
                root->nodes[i] = NULL;
            }
        }
        _FREE(root->nodes);
        root->nodes = NULL;
    }

    _FREE(root);
    */
    fprintf(stderr, "PRINT RESULT:\n");
    root->print(root, 0, "");


    return -1;
}

int Parser::consume(Parser *self, const char *str)
{
    if (memcmp(self->cursor->s, str, self->cursor->sLen) == 0)
    {
        //self->cursor = self->cursor->next;
        self->cursor = self->cursor + 1;
        return TRUE;
    }
    PrintError("Syntax error: expected %s got %.*s\n",
        str, (int) self->cursor->sLen, (const char *) self->cursor->s);
    return FALSE;
}

int Parser::consumeOptional(Parser *self, const char *str)
{
    if (memcmp(self->cursor->s, str, self->cursor->sLen) == 0)
    {
        //self->cursor = self->cursor->next;
        self->cursor = self->cursor + 1;
        return TRUE;
    }

    return FALSE;
}


void Parser::backtrack(Parser *self, int offset)
{
    // TODO: Might iterate over a linked list, maybe.
    self->cursor = self->cursor - offset;
}

int Parser::consumeGo(Parser* self)
{
    //self->cursor = self->cursor->next;
    self->cursor = self->cursor + 1;
    return 0;
}

int Parser::peek(Parser *self, const char *str, int offsetAhead)
{
    Token* ahead = self->cursor + offsetAhead;


    int len = CstrLen(str);
    if (str == NULL || ahead->sLen < len)
    {
        return FALSE;
    }

    if (memcmp(ahead->s, str, len) == 0)
    {
        return TRUE;
    }
    return FALSE;
}

int Parser::peekType(Parser *self, TokenKind kind, int offsetAhead)
{
    Token* ahead = self->cursor + offsetAhead;
    return (ahead->type == kind);
}
