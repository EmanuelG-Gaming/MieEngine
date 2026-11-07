#include "circularis.h"

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
        case AST_TYPE:
        case AST_FUNCTION_RET_TYPE:
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

        case AST_FUNCTION_CALL: return "AST_FUNCTION_CALL";

        case AST_BLOCK: return "AST_BLOCK";

        case AST_FACTOR: return "AST_FACTOR";

        case AST_OP_ADD: return "AST_OP_ADD";
        case AST_OP_SUB: return "AST_OP_SUB";
        case AST_OP_MUL: return "AST_OP_MUL";
        case AST_OP_DIV: return "AST_OP_DIV";
        case AST_OP_MOD: return "AST_OP_MOD";

        case AST_OP_BW_XOR: return "AST_OP_BW_XOR";
        case AST_OP_BW_OR: return "AST_OP_BW_OR";
        case AST_OP_BW_AND: return "AST_OP_BW_AND";

        case AST_OP_BW_SHL: return "AST_OP_BW_SHL";
        case AST_OP_BW_SHR: return "AST_OP_BW_SHR";

        case AST_VAR_DECL: return "AST_VAR_DECL";
        case AST_VAR_DEF: return "AST_VAR_DEF";

        case AST_GLOBAL_DECL: return "AST_GLOBAL_DECL";
        case AST_GLOBAL_DEF: return "AST_GLOBAL_DEF";

        case AST_ASSIGN: return "AST_ASSIGN";
        case AST_RETURN_STMT: return "AST_RETURN_STMT";
        case AST_IF_STMT: return "AST_IF_STMT";
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

    if (!hasType)
    {
        PrintError("Syntax error: type is not valid!\n");
        return NULL;
    }

    Token* prevToken = self->cursor;
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
    a->value.type = AST_STRING;
    a->value.str = prevToken->s;
    a->value.sLen = prevToken->sLen;

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
        // TODO: Return an error index, maybe.
        // And only report the error if it's a syntactic one.

        //PrintError("Syntax error: Expected '='.\n");
        return NULL;
    }

    AST_Node* c = expr(self, node);
    if (c == NULL)
    {
        //PrintError("Syntax error: Expected expression.\n");
        return NULL;
    }
    node->C = c;
    node->C->parent = node;

    if (!consumeOptional(self, ";"))
    {
        //PrintError("Syntax error: Expected semicolon ;\n");
        return NULL;
    }

    return node;
}

AST_Node* Parser::varDecl(Parser *self, AST_Node *parent)
{
    if (consumeOptional(self, "global"))
    {
        PrintError("Syntax error: Cannot declare a local variable with global!\n");
        return NULL;
    }

    AST_Node* node = (AST_Node *) _CALLOC(1, sizeof(AST_Node));
    assert(node != NULL);
    node->type = AST_VAR_DECL;
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

AST_Node* Parser::varDef(Parser *self, AST_Node *parent)
{
    if (consumeOptional(self, "global"))
    {
        PrintError("Syntax error: Cannot use types or 'global' in a local variable definition!");
        return NULL;
    }

    AST_Node* node = (AST_Node *) _CALLOC(1, sizeof(AST_Node));
    assert(node != NULL);
    node->type = AST_VAR_DEF;
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
// assign -> name ("+"|"-"|"*"|"/")*"=" expr
AST_Node* Parser::assign(Parser *self, AST_Node *parent)
{
    // Do not parse types.
    if (peek(self, "int", 0) || peek(self, "float", 0) || peek(self, "string", 0) ||
        peek(self, "fn", 0) || peek(self, "void", 0) ||
        peek(self, "global", 0))
    {
        PrintError("Cannot assign a value to a type!\n");
        return NULL;
    }

    Token* prevToken = self->cursor;
    consumeGo(self);

    AST_Node* node = (AST_Node *) _CALLOC(1, sizeof(AST_Node));
    assert(node != NULL);
    node->type = AST_ASSIGN;
    node->parent = parent;

    AST_Node* a = (AST_Node *) _CALLOC(1, sizeof(AST_Node));
    assert(a != NULL);
    a->type = AST_IDENT;
    a->value.type = AST_STRING;
    a->value.str = prevToken->s;
    a->value.sLen = prevToken->sLen;

    node->A = a;
    node->A->parent = node->A;

    Token* currentToken = self->cursor;
    // Parse the first symbol.
    if (!consumeOptional(self, "="))
    {
        // If the first symbol is not an equals sign,
        // then it must be a compound assignment +=, -=, *=, /=, %=.
        consumeGo(self);
    }

    AST_Node* b = expr(self, node);
    if (b == NULL)
    {
        PrintError("Syntax error: Expected expression.\n");
        return NULL;
    }
    node->B = b;
    node->B->parent = node->B;

    return b;
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

    AST_Node* node = (AST_Node *) _CALLOC(1, sizeof(AST_Node));
    assert(node != NULL);
    node->type = AST_RETURN_STMT;
    node->parent = parent;

    AST_Node* a = expr(self, node);
    if (a == NULL)
    {
        PrintError("Syntax error: expected expression: %*s\n", TOKEN_FMT(self->cursor));
        return NULL;
    }
    node->A = a;
    node->A->parent = node;

    if (!consume(self, ";"))
    {
        PrintError("Syntax error: expected ';'.\n");
        return NULL;
    }

    return node;
}

// Either an INT, a FLOAT or an IDENT.
AST_Node* Parser::factor(Parser *self, AST_Node *parent)
{
    // It is a function call.
    if (peekType(self, TOKEN_IDENT, 0) && peek(self, "(", 1))
    {
        fprintf(stderr, "IT IS A FUNCTION CALL! TOKEN: %.*s\n", TOKEN_FMT(self->cursor));
        AST_Node* node = functionCall(self, parent);
        if (node == NULL)
        {
            PrintError("Syntax error: Expected function call!\n");
            return NULL;
        }
        node->parent = parent;

        fprintf(stderr, "TOKEN AFTER CALL: %.*s\n", TOKEN_FMT(self->cursor));

        return node;
    }

    if (peekType(self, TOKEN_INT_LIT, 0))
    {
        AST_Node* node = (AST_Node *) _CALLOC(1, sizeof(AST_Node));
        assert(node != NULL);
        node->type = AST_FACTOR;
        node->parent = parent;
        node->value.type = AST_INT;
        node->value.i = atoi(self->cursor->s);
        consumeGo(self);

        fprintf(stderr, "PARSING INT! %d\n", node->value.i);

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


// Precedence climbing algorhithm.
typedef struct OpMapping {
    TokenKind tokenType;
    AST_types astType;
} OpMapping;

typedef struct PrecedenceLevel {
    OpMapping operators[4];
    int opCount;
} PrecedenceLevel;


#define N_PRECEDENCE_LEVELS 6
static const PrecedenceLevel precedenceChain[N_PRECEDENCE_LEVELS] = {
    // Level 0: expr (small precedence).
    {
        { { TOKEN_OP_BW_OR, AST_OP_BW_OR }, },
        1,
    },
    {
        { { TOKEN_OP_BW_XOR, AST_OP_BW_XOR } },
        1,
    },
    {
        { { TOKEN_OP_BW_AND, AST_OP_BW_AND } },
        1,
    },
    {
        {
            { TOKEN_OP_BW_SHL, AST_OP_BW_SHL },
            { TOKEN_OP_BW_SHR, AST_OP_BW_SHR },
        },
        2,
    },
    {
        {
            { TOKEN_OP_ADD, AST_OP_ADD },
            { TOKEN_OP_SUB, AST_OP_SUB },
        },
        2,
    },

    // Level 1: larger precedence.
    {
        {
            { TOKEN_OP_MUL, AST_OP_MUL },
            { TOKEN_OP_DIV, AST_OP_DIV },
            { TOKEN_OP_MOD, AST_OP_MOD },
        },
        3,
    },
};

AST_Node* Parser::binaryOps(Parser* self, AST_Node* parent, int level)
{
    if (level >= N_PRECEDENCE_LEVELS)
    {
        return factor(self, parent);
    }

    AST_Node* left = binaryOps(self, parent, level+1);
    if (left == NULL)
    {
        PrintError("Syntax error: Expected an expression for the LHS.\n");
        return NULL;
    }

    // Match and verify tokens.
    while (1)
    {
        Token* opToken = self->cursor;

        AST_types foundAstType = AST_NONE;
        for (int i = 0; i < precedenceChain[level].opCount; ++i)
        {
            if (opToken->type == precedenceChain[level].operators[i].tokenType)
            {
                foundAstType = precedenceChain[level].operators[i].astType;
                break;
            }
        }
        if (foundAstType == AST_NONE)
        {
            break;
        }

        consumeGo(self);

        AST_Node* opNode = (AST_Node *) _CALLOC(1, sizeof(AST_Node));
        assert(opNode != NULL);
        opNode->type = foundAstType;
        opNode->parent = parent;

        opNode->A = left;
        opNode->A->parent = opNode;

        // Parse RHS operator.
        AST_Node* right = binaryOps(self, opNode, level+1);
        if (right == NULL)
        {
            PrintError("Syntax error: Expected an expression for the RHS.\n");
            return NULL;
        }
        opNode->B = right;
        opNode->B->parent = opNode;


        // The current node becomes the left node,
        // in case the operation continues (like in "1 + 2 + 3").
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
// Operators with a higher precedence will be parsed later down the line.
// Operators like '+' and '-' will have a lower precendence compared to
// operators like '*' and '/'.

// The chain of precendence shall be something like this:
// logicalOR() -> logicalAND() -> bitwiseOR() -> ... -> expr() -> term() -> factor().

/*
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

err:
    return NULL;
}
*/

AST_Node* Parser::expr(Parser *self, AST_Node *parent)
{
    AST_Node* node = binaryOps(self, parent, 0);
    if (node == NULL)
    {
        PrintError("Syntax error: Couldn't parse binary operation!\n");
        return NULL;
    }

    return node;
}


// Grammar rule:
// block -> "{" (fnCall|returnStmt)* "}"
AST_Node* Parser::block(Parser* self, AST_Node *parent)
{
    // TODO: Code blocks without curly braces?
    int hasCurlyBraces = FALSE;
    /*
    if (consumeOptional(self, "{"))
    {
        hasCurlyBraces = TRUE;
    }
    */

    if (!consume(self, "{"))
    {
        PrintError("Syntax error: Expected closing curly brace: '}'\n");
        return NULL;
    }

    AST_Node* node = (AST_Node *) _CALLOC(1, sizeof(AST_Node));
    assert(node != NULL);

    node->type = AST_BLOCK;
    node->parent = parent;

    AST_Node* currentChild = NULL;
    while (!peek(self, "}", 0))
    {
        currentChild = NULL;
        Token* prevToken = self->cursor;

        if (peek(self, "return", 0))
        {
            // Return statements.
            self->cursor = prevToken;
            currentChild = returnStmt(self, node);
        }
        if (peek(self, "if", 0))
        {
            self->cursor = prevToken;
            currentChild = ifStmt(self, node);
        }

        if (currentChild == NULL)
        {
            // Function calls.
            self->cursor = prevToken;
            currentChild = functionCall(self, node);
            if (currentChild != NULL)
            {
                if (!consumeOptional(self, ";"))
                {
                    PrintError("Syntax error: Expected semicolon at the end of function call!\n");
                    return NULL;
                }
            }
        }


        // TODO: Fix priority in grammar rules.
        if (currentChild == NULL)
        {
            // Variable declarations.
            self->cursor = prevToken;
            currentChild = varDecl(self, node);
        }
        if (currentChild == NULL)
        {
            // Variable definitions.
            self->cursor = prevToken;
            currentChild = varDef(self, node);
        }
        if (currentChild == NULL)
        {
            // Variable assignments.
            self->cursor = prevToken;
            currentChild = assign(self, node);
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
    }
    if (!peek(self, "}", 0))
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
// ifStmt -> "if" "(" expr ")" block
AST_Node* Parser::ifStmt(Parser *self, AST_Node *parent)
{
    if (!consume(self, "if"))
    {
        PrintError("Syntax error: Expected 'if' token.\n");
        return NULL;
    }

    // Comment this for R*st syntax.
    /*
    if (!consume(self, "("))
    {
        PrintError("Syntax error: Expected opening '(' parenthesis.\n");
        return NULL;
    }
    */

    AST_Node* node = (AST_Node *) _CALLOC(1, sizeof(AST_Node));
    assert(node != NULL);
    node->type = AST_IF_STMT;
    node->parent = parent;

    AST_Node* a = expr(self, node);
    if (a == NULL)
    {
        PrintError("Syntax error: Expected expression.\n");
        return NULL;
    }
    node->A = a;
    node->A->parent = node;

    /*
    if (!consume(self, ")"))
    {
        PrintError("Syntax error: Expected closing ')' parenthesis.\n");
        return NULL;
    }
    */


    AST_Node* b = block(self, node);
    if (b == NULL)
    {
        PrintError("Syntax error: Expected if statement block.\n");
        return NULL;
    }
    node->B = b;
    node->B->parent = node;


    return node;
}


// Grammar rule:
// functionCall -> IDENT "(" ((expr|functionCall) ",")* ")" ";"
AST_Node* Parser::functionCall(Parser* self, AST_Node* parent)
{
    // h moment.
    int hasType = 0;
    if(peek(self, "int", 0)) {
        hasType = 1;
    } else if (peek(self, "float", 0)) {
        hasType = 1;
    } else if (peek(self, "string", 0)) {
        hasType = 1;
    } else if (peek(self, "void", 0)) {
        hasType = 1;
    } else if (peek(self, "fn", 0)) {
        hasType = 1;
    }

    if (hasType)
    {
        PrintError("Syntax error: Cannot have a function call named after a type! %.*s\n", TOKEN_FMT(self->cursor));
        return NULL;
    }

    AST_Node* node = (AST_Node *) _CALLOC(1, sizeof(AST_Node));
    assert(node != NULL);
    node->type = AST_FUNCTION_CALL;
    node->parent = parent;

    Token* prevToken = self->cursor;
    consumeGo(self);

    AST_Node* a = (AST_Node *) _CALLOC(1, sizeof(AST_Node));
    assert(a != NULL);
    a->type = AST_FUNCTION_IDENT;
    a->parent = node;
    a->value.type = AST_STRING;
    a->value.str = prevToken->s;
    a->value.sLen = prevToken->sLen;

    node->A = a;
    node->A->parent = node;

    //consumeGo(self);
    consume(self, "(");

    //fprintf(stderr, "Current token !!! : %.*s\n", TOKEN_FMT(self->cursor));

    AST_Node* currentParam = NULL;
    while (!peek(self, ")", 0))
    {
        currentParam = NULL;
        currentParam = expr(self, node);
        if (currentParam == NULL)
        {
            PrintError("Syntax error: Expected expression.\n");
            return NULL;
        }

        // Add static vector.
        if (node->nodes == NULL)
        {
            size_t bytecount = sizeof(AST_Node *) * NODE_CAPACITY;
            node->nodes = (AST_Node **) _MALLOC(bytecount);
        }
        node->nodes[node->nodeCount] = currentParam;
        node->nodeCount++;

        consumeOptional(self, ",");
    }

    if (!consume(self, ")"))
    {
        return NULL;
    }
    /*
    if (!consumeOptional(self, ";"))
    {
        PrintError("Syntax error: Expected semicolon at the end of function call!\n");
        return NULL;
    }
    */
    //consumeOptional(self, ";");

    return node;
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
    Token* prevToken = self->cursor;
    consumeGo(self);
    if (peek(self, "int", 0) || peek(self, "float", 0) || peek(self, "string", 0))
    {
        PrintError("Syntax error: cannot have multiple variable types!\n");
        return NULL;
    }

    AST_Node* node = (AST_Node *) _CALLOC(1, sizeof(AST_Node));
    assert(node != NULL);
    node->type = AST_FUNCTION_ARG;
    node->parent = parent;

    // TODO:

    // Get the TYPE token and put it in the tree.
    AST_Node* a = (AST_Node *) _CALLOC(1, sizeof(AST_Node));
    assert(a != NULL);
    a->type = AST_TYPE;
    a->parent = node;
    a->value.type = AST_STRING;
    a->value.str = prevToken->s;
    a->value.sLen = prevToken->sLen;

    node->A = a;
    node->A->parent = node;

    // Get the NAME token.
    AST_Node* b = (AST_Node *) _CALLOC(1, sizeof(AST_Node));
    assert(b != NULL);
    b->type = AST_IDENT;
    b->parent = node;
    b->value.type = AST_STRING;
    b->value.str = self->cursor->s;
    b->value.sLen = self->cursor->sLen;

    node->B = b;
    node->B->parent = node;

    consumeGo(self);
    consumeOptional(self, ",");

    return node;
}

// Grammar rule:
// functionDeclaration -> type NAME "(" (varDecl ",")* ")"
AST_Node* Parser::functionDeclaration(Parser *self, AST_Node* parent)
{
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
        return NULL;
    }

    AST_Node* c = NULL;


    AST_Node* currentArg = NULL;
    int hasClosing = peek(self, ")", 0);
    while (!hasClosing)
    {
        // Parse function arguments.
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

    /*
    if (!consume(self, "{"))
    {
        PrintError("Syntax error: Expected opening curly brace: '{'\n");
        goto err;
    }
    */


    c = block(self, master);
    if (c == NULL)
    {
        goto err;
    }
    master->C = c;

    /*
    if (!consume(self, "}"))
    {
        PrintError("Syntax error: Expected closed curly brace: '}'\n");
        return NULL;
    }
    */



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
        currentNode = NULL;
        Token* prevCursor = self->cursor;
        if (currentNode == NULL)
        {
            self->cursor = prevCursor;
            currentNode = globalDecl(self, root);
        }

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
    /*
    PrintError("Syntax error: expected %s got %.*s\n",
        str, (int) self->cursor->sLen, (const char *) self->cursor->s);
    */

    /*
    PrintError("Syntax error: expected %s got %.*s\n",
        str, (int) self->cursor->sLen, (const char *) self->cursor->s);
    */

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
