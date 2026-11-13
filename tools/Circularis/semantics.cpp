#include "base/types.h"
#include "circularis.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <math.h>

#define AddToHashTable(arr, val, Tarr, Tval) \
   Tval signature; \
do { \
    int h = CalcPseudoHashBounded(val.name, val.nameLen, 128); \
    Tarr l = &arr[h];\
    signature = (Tval) _CALLOC(1, sizeof(val));\
    assert(signature != NULL);\
    SLL_APPEND_FRONT((l)->begin, (l)->end, signature);\
} while (0); \
    signature


#define HashTableFind(arr, val, Tarr, T) \
    T found = NULL; \
do { \
    int h = CalcPseudoHashBounded((val)->name, (val)->nameLen, 128); \
    Tarr l = &(arr)[h]; \
    for (T ptr = (l)->begin; ptr != NULL; ptr = (ptr)->next) { \
        if (ptr->nameLen == (val)->nameLen && (memcmp(ptr->name, (val)->name, ptr->nameLen) == 0)) { \
            found = ptr; \
            break; \
        } \
    }\
} while (0); \
    found;


#define HashTableFindNextDuplicate(arr, val, Tarr, Tval) \
    Tval found = NULL; \
do { \
    for (Tval ptr = (val)->next; ptr != NULL; ptr = ptr->next) { \
        if (ptr->nameLen == (val)->nameLen && (memcmp(ptr->name, (val)->name, ptr->nameLen) == 0)) { \
            found = ptr; \
            break; \
        } \
    } \
    if (found) break; \
    int h = CalcPseudoHashBounded((val)->name, val->nameLen, 128); \
    Tarr l = &arr[h]; \
    for (DECLTYPE_CAST(val) ptr = l->begin; ptr != NULL; ptr = ptr->next) \
    { \
        if (ptr == val) { \
            found = NULL; \
            break; \
        } \
        if (ptr->nameLen == (val)->nameLen && (memcmp(ptr->name, (val)->name, ptr->nameLen) == 0)) { \
            found = ptr; \
            break; \
        } \
    } \
} while (0); \
    found;




Semantics::Semantics()
{
    _MEMSET(this, 0, sizeof(Semantics));
}

Semantics::~Semantics()
{
}


void Semantics::init(Semantics* self, AST_Node* parent)
{
    self->parent = parent;
}

void Semantics::visitGlobalVars(Semantics *self, AST_Node *parent)
{
    if (parent->nodes != NULL)
    {
        for (int i = 0; i < parent->nodeCount; ++i)
        {
            AST_Node* node = parent->nodes[i];
            if (node->type == AST_GLOBAL_DECL)
            {
                if (node->A->type != AST_TYPE)
                {
                    fprintf(stderr, "Semantic error: Not a type!\n");
                    continue;
                }

                // Type system.
                AST_valueTypes vt;
                AST_value t = node->A->value;
                if (!memcmp(t.str, "int", t.sLen)) {
                    vt = AST_INT;
                } else if (!memcmp(t.str, "float", t.sLen)) {
                    vt = AST_FLOAT;
                } else if (!memcmp(t.str, "string", t.sLen)) {
                    vt = AST_STRING;
                } else {
                    fprintf(stderr, "Semantic error: Invalid type (%.*s) for global variable declaration!\n"
                            "Can only be int, float or string!\n", (int) t.sLen, (const char *) t.str);
                    continue;
                }
                VarSignature s;
                s.type = t.str;
                s.typeLen = t.sLen;
                s.name = node->B->value.str;
                s.nameLen = node->B->value.sLen;
                s.vt = vt;
                s.flags = node->flags;
                s.nestCount = 0;

                fprintf(stderr, "%s: global var decl: string: %.*s\n", __func__, (int) s.nameLen, s.name);

                VarSignatureList* lst = self->globalVarDecl;
                AddToHashTable(lst, s, VarSignatureList*, VarSignature*);
            }
            else if (node->type == AST_GLOBAL_DEF)
            {
                if (node->A->type != AST_TYPE)
                {
                    fprintf(stderr, "Semantic error: Not a type!\n");
                    continue;
                }

                // Type system.
                AST_valueTypes vt;
                AST_value t = node->A->value;
                if (!memcmp(t.str, "int", t.sLen)) {
                    vt = AST_INT;
                } else if (!memcmp(t.str, "float", t.sLen)) {
                    vt = AST_FLOAT;
                } else if (!memcmp(t.str, "string", t.sLen)) {
                    vt = AST_STRING;
                } else {
                    fprintf(stderr, "Semantic error: Invalid type %.*s for global variable definition!\n"
                            "Can only be int, float or string!\n", (int) t.sLen, (const char *) t.str);
                    continue;
                }

                VarSignature s;
                s.type = t.str;
                s.typeLen = t.sLen;
                s.name = node->B->value.str;
                s.nameLen = node->B->value.sLen;
                s.vt = vt;
                s.flags = node->flags;
                s.nestCount = 0;

                fprintf(stderr, "%s: global var def: string: %.*s\n", __func__, (int) s.nameLen, s.name);

                VarSignatureList* lst = self->globalVarDef;
                AddToHashTable(lst, s, VarSignatureList*, VarSignature*);
            }

        }
    }
}

int Semantics::checkGlobalVars(Semantics *self)
{
    // Cannot have two defined global variables with the same name.
    for (int i = 0; i < 128; ++i)
    {
        VarSignatureList lst = self->globalVarDecl[i];
        if (lst.begin)
        {
            VarSignature* duplicate = findNextDuplicateVarSignature(self, self->globalVarDecl, lst.begin);
            if (duplicate != NULL)
            {
                fprintf(stderr, "Semantic error: Cannot have duplicate global variable names!\n");
                return -1;
            }
        }
    }

    return 0;
}

/*
   Functions.
*/

void Semantics::visitFunctions(Semantics *self, AST_Node *parent)
{
    // Check for return types.
    if (parent->nodes != NULL)
    {
        fprintf(stderr, "Visiting functions!\n");
        // start -> (fnDecl | fnDef)
        for (int i = 0; i < parent->nodeCount; ++i)
        {
            AST_Node* node = parent->nodes[i];
            if (node->type == AST_FUNCTION_DECL)
            {
                if (node->B->type == AST_FUNCTION_IDENT)
                {
                    // A-> ret, B->name, List->args.
                    FunctionSignature decl;
                    decl.retType = node->A->value.str;
                    decl.retLen = node->A->value.sLen;
                    decl.name = node->B->value.str;
                    decl.nameLen = node->B->value.sLen;

                    decl.args.reserve(16);
                    for (int j = 0; j < node->nodeCount; ++j)
                    {
                        AST_Node* arg = node->nodes[j];
                        FunctionArgument t;
                        t.type.type = arg->A->value.type;
                        t.type.str = arg->A->value.str;
                        t.type.sLen = arg->A->value.sLen;

                        t.name.type = arg->B->value.type;
                        t.name.str = arg->B->value.str;
                        t.name.sLen = arg->B->value.sLen;


                        decl.args.emplace_back(t);
                        fprintf(stderr, "placing argument: %d\n", j);
                    }


                    fprintf(stderr, "%s: decl: string: %.*s\n", __func__, (int) node->B->value.sLen, node->B->value.str);
                    //self->functionDecl.insert({ node->B->value.str, decl});
                    //self->functionDecl[node->B->value.str] = decl;
                    //self->functionDecl.emplace_back(decl);

                    FunctionSignatureList* lst = self->functionDecl;
                    AddToHashTable(lst, decl, FunctionSignatureList*, FunctionSignature*);
                    signature->next = NULL;
                    signature->name = decl.name;
                    signature->nameLen = decl.nameLen;
                    signature->retType = decl.retType;
                    signature->retLen = decl.retLen;
                    signature->args = std::move(decl.args);
                }
                else
                {
                    fprintf(stderr, "Semantic error: Incorrect AST in function identifier!\n");
                    return;
                }
            }

            if (node->type == AST_FUNCTION_DEF)
            {
                if (node->B->type == AST_FUNCTION_IDENT)
                {
                    // Check if function decl corresponds to its def.
                    FunctionDefinition def;
                    def.retType = node->A->value.str;
                    def.retLen = node->A->value.sLen;
                    def.name = node->B->value.str;
                    def.nameLen = node->B->value.sLen;

                    def.args.reserve(16);
                    for (int j = 0; j < node->nodeCount; ++j)
                    {
                        AST_Node* arg = node->nodes[j];
                        FunctionArgument t;
                        t.type.type = arg->A->value.type;
                        t.type.str = arg->A->value.str;
                        t.type.sLen = arg->A->value.sLen;

                        t.name.type = arg->B->value.type;
                        t.name.str = arg->B->value.str;
                        t.name.sLen = arg->B->value.sLen;

                        def.args.emplace_back(t);
                    }

                    fprintf(stderr, "%s: def: string: %.*s\n", __func__, (int) node->B->value.sLen, node->B->value.str);

                    FunctionDefinitionList* lst = self->functionDef;
                    AddToHashTable(lst, def, FunctionDefinitionList*, FunctionDefinition*);
                    signature->next = NULL;
                    signature->name = def.name;
                    signature->nameLen = def.nameLen;
                    signature->retType = def.retType;
                    signature->retLen = def.retLen;
                    signature->args = std::move(def.args);
                    signature->parent = node->C;

                    //addToFunctionSignature(self, self->functionDef, def);

                    //self->functionDef.insert({ node->B->value.str, def});
                    //self->functionDef.insert_or_assign("among", def);
                    //self->functionDecl.emplace_back(def);
                }
                else
                {
                    fprintf(stderr, "Semantic error: Incorrect AST in function identifier!\n");
                    return;
                }

                if (node->C->type == AST_BLOCK)
                {
                    // Put function definitions.

                }
                else
                {
                    fprintf(stderr, "Semantic error: Incorrectly-defined function block!\n");
                    return;
                }
            }
        }
    }
}

void Semantics::visitFunctionCalls(Semantics *self, AST_Node *parent)
{
    // Visit by blocks.
    if (parent->nodes != NULL)
    {
        for (int i = 0; i < parent->nodeCount; ++i)
        {
            AST_Node* node = parent->nodes[i];

            if (node == NULL)
            {
                continue;
            }

            // TODO: Could visit from the main function first.
            if (node->type == AST_FUNCTION_DEF)
            {
                // Visit by block.
                if (node->C == NULL) continue;
                if (node->C->type != AST_BLOCK) continue;

                visitBlockFnCall(self, node->C);
            }
        }
    }

    // We then validate the functions then fill them up with their corresponding types.


}

void Semantics::visitFunctionsForBlock(Semantics *self, AST_Node *parent)
{
    {
        for (int i = 0; i < 128; ++i)
        {
            FunctionDefinitionList l = self->functionDef[i];
            if (l.begin)
            {
                for (FunctionDefinition* ptr = l.begin; ptr != NULL; ptr = ptr->next)
                {
                    // Call visit block.
                    visitBlock(self, ptr->parent, &ptr->body);
                }
            }
        }
    }
}

void Semantics::visitBlock(Semantics *self, AST_Node *parent, VarBlock *block)
{
    // We add to block.
    if (parent->A != NULL)
    {
        visitBlock(self, parent->A, block);
    }
    if (parent->B != NULL)
    {
        visitBlock(self, parent->B, block);
    }
    if (parent->C != NULL)
    {
        visitBlock(self, parent->C, block);
    }
    if (parent->D != NULL)
    {
        visitBlock(self, parent->D, block);
    }

    if (parent->nodes != NULL)
    {
        for (int i = 0; i < parent->nodeCount; ++i)
        {
            AST_Node* node = parent->nodes[i];
            if (node == NULL) continue;

            if (node->type == AST_BLOCK)
            {
                // Begin stack frame.
                VarBlock* child = (VarBlock *) _CALLOC(1, sizeof(VarBlock));
                assert(child != NULL);
                _MEMSET(&child->localVarsTable, 0, sizeof(child->localVarsTable));
                child->parent = block;

                block->children.emplace_back(child);

                visitBlock(self, node, child);
            }
            else if (node->type == AST_VAR_DECL)
            {
            }
            else if (node->type == AST_VAR_DEF)
            {
            }
        }

    }
}



void Semantics::visitBlockFnCall(Semantics *self, AST_Node *parent)
{
    for (int i = 0; i < parent->nodeCount; ++i)
    {
        AST_Node* node = parent->nodes[i];

        if (node->type == AST_FUNCTION_CALL)
        {
            // Add function call.
            FunctionSignature s;
            s.retType = NULL;
            s.retLen = 0;
            s.name = node->A->value.str;
            s.nameLen = node->A->value.sLen;

            s.args.reserve(16);
            for (int j = 0; j < node->nodeCount; ++j)
            {
                AST_Node* arg = node->nodes[j];
                FunctionArgument t;
                t.name.type = arg->value.type;
                switch (arg->value.type)
                {
                    case AST_INT: t.name.i = arg->value.i; fprintf(stderr, "argI: %d\n", t.name.i); break;
                    case AST_FLOAT: t.name.f = arg->value.f; fprintf(stderr, "argF: %f\n", t.name.f); break;
                    case AST_STRING:
                    {
                        t.name.str = arg->value.str;
                        t.name.sLen = arg->value.sLen;

                        fprintf(stderr, "argS: %.*s\n", t.name.sLen, t.name.str);
                        // We then fill out the types as we go.
                    } break;
                    default: break;
                }
                s.args.emplace_back(t);
            }
            fprintf(stderr, "Function call: %.*s\n", (int) s.nameLen, s.name);

            // Add function call signature.
            FunctionSignatureList* lst = self->functionCalls;
            AddToHashTable(lst, s, FunctionSignatureList*, FunctionSignature*);
            signature->next = NULL;
            signature->name = s.name;
            signature->nameLen = s.nameLen;
            signature->retType = s.retType;
            signature->retLen = s.retLen;
        }

        if (node->type == AST_BLOCK)
        {
            visitBlockFnCall(self, node);
        }
    }
}

int Semantics::checkFnSignatures(Semantics *self)
{
    int hasError = 0;

    // Cannot have two defined functions with the same name
    // (prevents function overloading).
    {
        FunctionDefinitionList* lst = self->functionDef;

        for (int i = 0; i < 128; ++i)
        {
            FunctionDefinitionList lstDef = self->functionDef[i];
            if (lstDef.begin)
            {
                FunctionDefinition* s = lstDef.begin;
                HashTableFindNextDuplicate(lst, s, FunctionDefinitionList*, FunctionDefinition*);

                //FunctionSignature* duplicate = findNextDuplicateFnSignature(self, self->functionDef, lstDef.begin);
                if (found != NULL)
                {
                    fprintf(stderr, "Semantic error: Cannot have duplicate function definition names!\n");
                    return -1;
                }
            }
        }
    }

    // TODO: Could optimize this by iterating over the lists.

    {
        FunctionSignatureList* lst = self->functionDecl;

        // Return type checking.
        for (int i = 0; i < 128; ++i)
        {
            FunctionSignatureList lstDecl = self->functionDecl[i];
            for (FunctionSignature* decl = lstDecl.begin; decl != NULL; decl = decl->next)
            {
                //FunctionSignature* found = self->findFunctionSignature(self, self->functionDecl, decl);
                HashTableFind(lst, decl, FunctionSignatureList*, FunctionSignature*);

                if (found != NULL)
                {
                    if (found->retLen != decl->retLen || (memcmp(found->retType, decl->retType, decl->retLen) != 0))
                    {
                        // Return types do not match.
                        fprintf(stderr, "Semantic error: Function declaration return types do not match!\n");
                        //hasError |= (1 << 1);
                        //continue;
                        return -1;
                    }
                }
            }
        }
    }

    {
        FunctionDefinitionList* lst = self->functionDef;
        for (int i = 0; i < 128; ++i)
        {
            FunctionDefinitionList lstDef = self->functionDef[i];
            for (FunctionDefinition* def = lstDef.begin; def != NULL; def = def->next)
            {
                //FunctionSignature* found = self->findFunctionSignature(self, self->functionDef, def);
                HashTableFind(lst, def, FunctionDefinitionList*, FunctionDefinition*);

                if (found != NULL)
                {
                    if (found->retLen != def->retLen || (memcmp(found->retType, def->retType, def->retLen) != 0))
                    {
                        // Return types do not match.
                        fprintf(stderr, "Semantic error: Function definition return types do not match!\n");
                        //hasError |= (1 << 1);
                        return -1;
                    }
                }
            }
        }
    }

    // Count by all function decls.
    {
        FunctionDefinitionList* lst = self->functionDef;
        for (int i = 0; i < 128; ++i)
        {
            // Find definition by name.
            FunctionSignatureList lstDecl = self->functionDecl[i];
            for (FunctionSignature* decl = lstDecl.begin; decl != NULL; decl = decl->next)
            {
                //FunctionSignature* found = self->findFunctionSignature(self, self->functionDef, decl);

                // TODO: This might crash.
                HashTableFind(lst, decl, FunctionDefinitionList*, FunctionDefinition*);

                if (found == NULL)
                {
                    fprintf(stderr, "Semantic error: A function definition does not exist for the declaration! (%.*s)\n",
                            (int) decl->nameLen, decl->name);
                    return -1;
                }
            }
        }
    }

    // And then the args mismatching.
    {
        FunctionDefinitionList* lst = self->functionDef;

        for (int i = 0; i < 128; ++i)
        {
            FunctionSignatureList lstDecl = self->functionDecl[i];
            for (FunctionSignature* decl = lstDecl.begin; decl != NULL; decl = decl->next)
            {
                //FunctionSignature* found = self->findFunctionSignature(self, self->functionDef, decl);
                HashTableFind(lst, decl, FunctionDefinitionList*, FunctionDefinition*);

                if (found != NULL)
                {
                    // Check by size.
                    size_t size1 = decl->args.size();
                    size_t size2 = found->args.size();

                    if (size1 != size2)
                    {
                        fprintf(stderr, "Semantic error: Function argument sizes are different! (%zu for decl, %zu for def)\n",
                                size1, size2);
                        return -1;
                    }

                    // Check by type.
                    for (int i = 0; i < size1; ++i)
                    {
                        FunctionArgument* arg1 = &decl->args[i];
                        FunctionArgument* arg2 = &found->args[i];

                        if (arg1->type.sLen != arg2->type.sLen || (memcmp(arg1->type.str, arg2->type.str, arg1->type.sLen) != 0))
                        {
                            fprintf(stderr, "Semantic error: Type mismatch in function!\n");
                            continue;
                        }
                    }
                }
            }
        }
    }

    return 0;
}

/*
void Semantics::visitFunctionDefs(Semantics* self, AST_Node* parent)
{
    for (int i = 0; i < parent->nodeCount; ++i)
    {
        AST_Node* node = parent->nodes[i];
        if (node->type == AST_FUNCTION_DEF)
        {
            FunctionDefinition func;
            func.signature.name = parent->B->value.str;
            func.signature.nameLen = parent->B->value.sLen;
            func.signature.retType = parent->A->value.str;
            func.signature.retLen = parent->A->value.sLen;

            visitBlock(self, parent->C, &func.body);

        }
    }
}
*/


void Semantics::visitExprs(Semantics *self, AST_Node *parent)
{
    if (parent->A != NULL)
    {
        visitExprs(self, parent->A);
    }
    if (parent->B != NULL)
    {
        visitExprs(self, parent->B);
    }
    if (parent->C != NULL)
    {
        visitExprs(self, parent->C);
    }
    if (parent->D != NULL)
    {
        visitExprs(self, parent->D);
    }

    if (parent->nodes != NULL)
    {
        for (int i = 0; i < parent->nodeCount; ++i)
        {
            AST_Node* node = parent->nodes[i];
            if (node != NULL)
            {
                visitExprs(self, node);
            }
        }
    }

    // Do constant folding.
    if ((parent->type >= AST_OP_MIN) && (parent->type <= AST_OP_MAX))
    {
        AST_Node* node = parent;
        if (doConstFold(self, &node, NULL, NULL))
        {
            fprintf(stderr, "Semantic error: Couldn't do constant folding on expr!\n");
            return;
        }

    }
}

int Semantics::validate(Semantics *self)
{
    // Visit global variables.
    self->visitGlobalVars(self, gCompiler->parser.root);
    if (self->checkGlobalVars(self))
    {
        return -1;
    }

    // Visit functions.
    self->visitFunctions(self, gCompiler->parser.root);

    if (self->checkFnSignatures(self))
    {
        return -1;
    }

    // Visit local variables.


    self->visitExprs(self, gCompiler->parser.root);
    fprintf(stderr, "Nodes after being const-folded:\n");
    gCompiler->parser.root->print(gCompiler->parser.root, 0, "");

    self->visitFunctionCalls(self, gCompiler->parser.root);

    return 0;
}


AST_value Semantics::evaluateExpr(Semantics *self, AST_Node *node)
{
    if (node->type == AST_FACTOR)
    {
        return node->value;
    }

    // Evaluate left and right.
    AST_value lhs = evaluateExpr(self, node->A);
    AST_value rhs = evaluateExpr(self, node->B);

    if (lhs.type == AST_STRING)
    {
        fprintf(stderr, "Couldn't evaluate string LHS operand!\n");
        return CLITERAL(AST_value) {};
    }
    if (rhs.type == AST_STRING)
    {
        fprintf(stderr, "Couldn't evaluate string RHS operand!\n");
        return CLITERAL(AST_value) {};
    }

    int hasFloat = FALSE;

    // Conversion to float.
    if (lhs.type == AST_FLOAT)
    {
        rhs.f = (float) rhs.i;
        rhs.type = AST_FLOAT;
        hasFloat = TRUE;
    }
    else if (rhs.type == AST_FLOAT)
    {
        lhs.f = (float) rhs.f;
        lhs.type = AST_FLOAT;
        hasFloat = TRUE;
    }

    if (hasFloat)
    {
        float result = 0.0f;
        switch (node->type)
        {
            case AST_OP_ADD: result = lhs.f + rhs.f; break;
            case AST_OP_SUB: result = lhs.f - rhs.f; break;
            case AST_OP_MUL: result = lhs.f * rhs.f; break;
            case AST_OP_DIV: result = (rhs.f == 0) ? (100000.0f) : lhs.f / rhs.f; break;
            case AST_OP_MOD: result = (rhs.f == 0) ? (0.0f) : fmod(lhs.f, rhs.f); break;
            default:
            {
                fprintf(stderr, "Warning: Tried to add a non-arithmetic operation to floats!\n");
                AST_value ret;
                ret.type = AST_FLOAT;
                ret.f = 0.0f;
                return ret;
            } break;
        }

        AST_value ret;
        ret.type = AST_FLOAT;
        ret.f = result;
        return ret;
    }
    else
    {
        // Integers.
        int result = 0;
        switch (node->type)
        {
            case AST_OP_ADD: result = lhs.i + rhs.i; break;
            case AST_OP_SUB: result = lhs.i - rhs.i; break;
            case AST_OP_MUL: result = lhs.i * rhs.i; break;
            case AST_OP_DIV: result = (lhs.i == 0) ? (100000) : lhs.i / rhs.i; break;
            case AST_OP_MOD: result = (lhs.i == 0) ? (0) : lhs.i % rhs.i; break;

            case AST_OP_BW_XOR: result = lhs.i ^ rhs.i; break;
            case AST_OP_BW_OR: result = lhs.i | rhs.i; break;
            case AST_OP_BW_AND: result = lhs.i & rhs.i; break;

            case AST_OP_BW_SHL: result = lhs.i << rhs.i; break;
            case AST_OP_BW_SHR: result = lhs.i >> rhs.i; break;

            default:
            {
                fprintf(stderr, "Warning: Tried to add non-operator to integer operands!\n");
                AST_value ret;
                ret.type = AST_INT;
                ret.i = 0;
                return ret;
            } break;
        }
        AST_value ret;
        ret.type = AST_INT;
        ret.i = result;
        return ret;
    }
}

// Returns 0 if the node is foldeable.
// Returns -1 otherwise.
int Semantics::doConstFold(Semantics *self, AST_Node **np,
    VarSignatureList* localVars, VarSignatureList* globalVars)
{
    AST_Node* node = *np;
    if (node->type == AST_FACTOR)
    {
        return 0;
    }
    /*
    // Search in the constant tables.
    {
        VarSignature s;
        s.name = node->A->value.str;
        s.nameLen = node->A->value.sLen;
        {
            HashTableFind(globalVars, &s);
            if (found != NULL && found->flags == AST_FLAG_CONST)
            {
                return 0;
            }
        }
        {
            HashTableFind(localVars, &s);
            if (found != NULL && found->flags == AST_FLAG_CONST)
            {
                return 0;
            }
        }
    }

    {
        VarSignature s;
        s.name = node->B->value.str;
        s.nameLen = node->B->value.sLen;
        {
            HashTableFind(globalVars, &s);
            if (found != NULL && found->flags == AST_FLAG_CONST)
            {
                return 0;
            }
        }
        {
            HashTableFind(localVars, &s);
            if (found != NULL && found->flags == AST_FLAG_CONST)
            {
                return 0;
            }
        }
    }
    */




    if ((doConstFold(self, &node->A, localVars, globalVars) == -1) ||
        (doConstFold(self, &node->B, localVars, globalVars) == -1))
    {
        return -1;
    }


    // Evaluate left and right.
    AST_value lhs = evaluateExpr(self, node->A);
    AST_value rhs = evaluateExpr(self, node->B);
    AST_value ret;

    if (lhs.type == AST_STRING)
    {
        fprintf(stderr, "Couldn't evaluate string LHS operand!\n");
        return -1;
    }
    if (rhs.type == AST_STRING)
    {
        fprintf(stderr, "Couldn't evaluate string RHS operand!\n");
        return -1;
    }

    int hasFloat = FALSE;

    // Conversion to float.
    if (lhs.type == AST_FLOAT)
    {
        rhs.f = (float) rhs.i;
        rhs.type = AST_FLOAT;
        hasFloat = TRUE;
    }
    else if (rhs.type == AST_FLOAT)
    {
        lhs.f = (float) lhs.i;
        lhs.type = AST_FLOAT;
        hasFloat = TRUE;
    }

    if (hasFloat)
    {
        float result = 0.0f;
        switch (node->type)
        {
            case AST_OP_ADD: result = lhs.f + rhs.f; break;
            case AST_OP_SUB: result = lhs.f - rhs.f; break;
            case AST_OP_MUL: result = lhs.f * rhs.f; break;
            case AST_OP_DIV: result = (rhs.f == 0) ? (100000.0f) : lhs.f / rhs.f; break;
            case AST_OP_MOD: result = (rhs.f == 0) ? (0.0f) : fmod(lhs.f, rhs.f); break;
            default:
            {
                fprintf(stderr, "Warning: Tried to add a non-arithmetic operation to floats!\n");
                return -1;
            } break;
        }

        ret.type = AST_FLOAT;
        ret.f = result;
    }
    else
    {
        // Integers.
        int result = 0;
        switch (node->type)
        {
            case AST_OP_ADD: result = lhs.i + rhs.i; break;
            case AST_OP_SUB: result = lhs.i - rhs.i; break;
            case AST_OP_MUL: result = lhs.i * rhs.i; break;
            case AST_OP_DIV: result = (lhs.i == 0) ? (100000) : lhs.i / rhs.i; break;
            case AST_OP_MOD: result = (lhs.i == 0) ? (0) : lhs.i % rhs.i; break;

            case AST_OP_BW_XOR: result = lhs.i ^ rhs.i; break;
            case AST_OP_BW_OR: result = lhs.i | rhs.i; break;
            case AST_OP_BW_AND: result = lhs.i & rhs.i; break;

            case AST_OP_BW_SHL: result = lhs.i << rhs.i; break;
            case AST_OP_BW_SHR: result = lhs.i >> rhs.i; break;

            default:
            {
                fprintf(stderr, "Warning: Tried to add non-operator to integer operands!\n");
                return -1;
            } break;
        }
        ret.type = AST_INT;
        ret.i = result;
    }

    AST_Node* newNode = (AST_Node *) _CALLOC(1, sizeof(AST_Node));
    assert(newNode != NULL);
    newNode->type = AST_FACTOR;
    newNode->parent = node->parent;
    newNode->value = ret;

    node->parent->A = newNode;

    *np = newNode;
    node->terminate(node);

    return 0;
}

/*
   Visibility domains.
*/

void Semantics::printBlock(Semantics* self, VarBlock* block)
{
}
void Semantics::terminateBlock(Semantics* self, VarBlock* block)
{
    for (auto& child : block->children)
    {
        if (child != NULL)
        {
            terminateBlock(self, child);
            _FREE(child);
            child = NULL;
        }
    }
}


/*
   Var hash table.
*/

int Semantics::addToVarSignature(Semantics *self, VarSignatureList *arr, VarSignature var)
{
    int h = CalcPseudoHashBounded(var.name, var.nameLen, 128);
    VarSignatureList* lst = &arr[h];

    VarSignature* signature = (VarSignature *) _CALLOC(1, sizeof(VarSignature));
    assert(signature != NULL);
    *signature = var;
    /*
    signature->next = NULL;
    signature->type = var.type;
    signature->typeLen = var.typeLen;
    signature->name = var.name;
    signature->nameLen = var.nameLen;
    */

    SLL_APPEND_FRONT(lst->begin, lst->end, signature);

    return 0;
}


VarSignature* Semantics::findVarSignature(Semantics *self, VarSignatureList *arr, VarSignature* src)
{
    int h = CalcPseudoHashBounded(src->name, src->nameLen, 128);

    VarSignatureList* lst = &arr[h];
    for (VarSignature* ptr = lst->begin; ptr != NULL; ptr = ptr->next)
    {
        if (ptr->nameLen == src->nameLen && (memcmp(ptr->name, src->name, ptr->nameLen) == 0))
        {
            return ptr;
        }
    }

    return NULL;
}

VarSignature* Semantics::findNextDuplicateVarSignature(
    Semantics* self, VarSignatureList* arr,
    VarSignature* src)
{
    for (VarSignature* ptr = src->next; ptr != NULL; ptr = ptr->next)
    {
        if (ptr->nameLen == src->nameLen && (memcmp(ptr->name, src->name, ptr->nameLen) == 0))
        {
            return ptr;
        }
    }

    // Otherwise, the duplicated element might be before the source node.
    int h = CalcPseudoHashBounded(src->name, src->nameLen, 128);
    VarSignatureList* lst = &arr[h];
    for (VarSignature* ptr = lst->begin; ptr != NULL; ptr = ptr->next)
    {
        if (ptr == src)
        {
            return NULL;
        }
        if (ptr->nameLen == src->nameLen && (memcmp(ptr->name, src->name, ptr->nameLen) == 0))
        {
            return ptr;
        }
    }

    return NULL;
}


/*
   Function hash table.
*/

int Semantics::addToFunctionSignature(Semantics* self, FunctionSignatureList* arr, FunctionSignature fn)
{
    int h = CalcPseudoHashBounded(fn.name, fn.nameLen, 128);
    // Put at hash.
    FunctionSignatureList* lst = &arr[h];

    FunctionSignature* signature = (FunctionSignature *) _CALLOC(1, sizeof(FunctionSignature));
    assert(signature != NULL);
    signature->next = NULL;
    signature->name = fn.name;
    signature->nameLen = fn.nameLen;
    signature->retType = fn.retType;
    signature->retLen = fn.retLen;
    signature->args = std::move(fn.args);

    SLL_APPEND_FRONT(lst->begin, lst->end, signature);

    return 0;
}



FunctionSignature* Semantics::findFunctionSignature(Semantics *self, FunctionSignatureList *arr, FunctionSignature* src)
{
    int h = CalcPseudoHashBounded(src->name, src->nameLen, 128);

    FunctionSignatureList* lst = &arr[h];
    for (FunctionSignature* ptr = lst->begin; ptr != NULL; ptr = ptr->next)
    {
        if (ptr->nameLen == src->nameLen && (memcmp(ptr->name, src->name, ptr->nameLen) == 0))
        {
            return ptr;
        }
    }

    return NULL;
}


FunctionSignature* Semantics::findNextDuplicateFnSignature(
    Semantics* self, FunctionSignatureList* arr,
    FunctionSignature* src)
{
    for (FunctionSignature* ptr = src->next; ptr != NULL; ptr = ptr->next)
    {
        if (ptr->nameLen == src->nameLen && (memcmp(ptr->name, src->name, ptr->nameLen) == 0))
        {
            return ptr;
        }
    }

    // Otherwise, the duplicated element might be before the source node.
    int h = CalcPseudoHashBounded(src->name, src->nameLen, 128);
    FunctionSignatureList* lst = &arr[h];
    for (FunctionSignature* ptr = lst->begin; ptr != NULL; ptr = ptr->next)
    {
        if (ptr == src)
        {
            return NULL;
        }
        if (ptr->nameLen == src->nameLen && (memcmp(ptr->name, src->name, ptr->nameLen) == 0))
        {
            return ptr;
        }
    }

    return NULL;
}

