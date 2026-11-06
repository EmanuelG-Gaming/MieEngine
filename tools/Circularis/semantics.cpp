#include "base/types.h"
#include "circularis.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <utility>


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
    // Init tables.
    //self->functionDef.init(&self->functionDef, TABLE_CAPACITY);
    //self->functionDecl.init(&self->functionDecl, TABLE_CAPACITY);
}


/*
    if (parent->A != NULL)
    {
        // Visit that parent.
        visitFunctions(self, parent->A);
    }
    if (parent->B != NULL)
    {
        visitFunctions(self, parent->B);
    }
    if (parent->C != NULL)
    {
        visitFunctions(self, parent->C);
    }
    if (parent->D != NULL)
    {
        visitFunctions(self, parent->D);
    }

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
                    for (int i = 0; i < node->nodeCount; ++i)
                    {
                        AST_Node* arg = node->nodes[i];
                        FunctionArgument t;
                        t.type = arg->A->value.str;
                        t.typeLen = arg->A->value.sLen;
                        t.name = arg->A->value.str;
                        t.nameLen = arg->A->value.sLen;

                        decl.args.emplace_back(t);
                        fprintf(stderr, "placing argument: %d\n", i);
                    }


                    fprintf(stderr, "%s: decl: string: %.*s\n", __func__, (int) node->B->value.sLen, node->B->value.str);
                    //self->functionDecl.insert({ node->B->value.str, decl});
                    //self->functionDecl[node->B->value.str] = decl;
                    //self->functionDecl.emplace_back(decl);

                    addToFunctionSignature(self, self->functionDecl, decl);
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
                    FunctionSignature def;
                    def.retType = node->A->value.str;
                    def.retLen = node->A->value.sLen;
                    def.name = node->B->value.str;
                    def.nameLen = node->B->value.sLen;

                    def.args.reserve(16);
                    for (int i = 0; i < node->nodeCount; ++i)
                    {
                        AST_Node* arg = node->nodes[i];
                        FunctionArgument t;
                        t.type = arg->A->value.str;
                        t.typeLen = arg->A->value.sLen;
                        t.name = arg->A->value.str;
                        t.nameLen = arg->A->value.sLen;

                        def.args.emplace_back(t);
                    }

                    fprintf(stderr, "%s: def: string: %.*s\n", __func__, (int) node->B->value.sLen, node->B->value.str);

                    addToFunctionSignature(self, self->functionDef, def);

                    //self->functionDef.insert({ node->B->value.str, def});
                    //self->functionDef.insert_or_assign("among", def);
                    //self->functionDecl.emplace_back(def);
                }
                else
                {
                    fprintf(stderr, "Semantic error: Incorrect AST in function identifier!\n");
                    return;
                }
            }
        }
    }
}

int Semantics::checkFnSignatures(Semantics *self)
{
    int hasError = 0;

    // Cannot have two defined functions with the same name
    // (prevents function overloading).
    for (int i = 0; i < 128; ++i)
    {
        FunctionSignatureList lstDef = self->functionDef[i];
        if (lstDef.begin)
        {
            FunctionSignature* duplicate = findNextDuplicate(self, self->functionDef, lstDef.begin);
            if (duplicate != NULL)
            {
                fprintf(stderr, "Semantic error: Cannot have duplicate function definition names!\n");
                return -1;
            }
        }
    }

    // Return type checking.
    for (int i = 0; i < 128; ++i)
    {
        FunctionSignatureList lstDecl = self->functionDecl[i];
        for (FunctionSignature* decl = lstDecl.begin; decl != NULL; decl = decl->next)
        {
            FunctionSignature* found = self->findFunctionSignature(self, self->functionDecl, decl);
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
    for (int i = 0; i < 128; ++i)
    {
        FunctionSignatureList lstDef = self->functionDef[i];
        for (FunctionSignature* def = lstDef.begin; def != NULL; def = def->next)
        {
            FunctionSignature* found = self->findFunctionSignature(self, self->functionDef, def);
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

    // Count by all function decls.
    for (int i = 0; i < 128; ++i)
    {
        // Find definition by name.
        FunctionSignatureList lstDecl = self->functionDecl[i];
        for (FunctionSignature* decl = lstDecl.begin; decl != NULL; decl = decl->next)
        {
            FunctionSignature* found = self->findFunctionSignature(self, self->functionDef, decl);
            if (found == NULL)
            {
                fprintf(stderr, "Semantic error: A function definition (%.*s) does not exist for the declaration!\n",
                        (int) decl->nameLen, decl->name);
                return -1;
            }
        }
    }

    // And then the args mismatching.
    for (int i = 0; i < 128; ++i)
    {
        FunctionSignatureList lstDecl = self->functionDecl[i];
        for (FunctionSignature* decl = lstDecl.begin; decl != NULL; decl = decl->next)
        {
            FunctionSignature* found = self->findFunctionSignature(self, self->functionDef, decl);

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

                    if (arg1->typeLen != arg2->typeLen || (memcmp(arg1->type, arg2->type, arg1->typeLen) != 0))
                    {
                        fprintf(stderr, "Semantic error: Type mismatch in function!\n");
                        continue;
                    }
                }
            }
        }
    }

    return 0;
}

int Semantics::validate(Semantics *self)
{
    self->visitFunctions(self, gCompiler->parser.root);
    if (self->checkFnSignatures(self))
    {
        return -1;
    }

    return 0;
}

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
        /*
        if (ptr == src)
        {
            return ptr;
        }
        */

        if (ptr->nameLen == src->nameLen && (memcmp(ptr->name, src->name, ptr->nameLen) == 0))
        {
            //fprintf(stderr, "FOUND11\n");
            return ptr;
        }
    }

    return NULL;
}

FunctionSignature* Semantics::findNextDuplicate(
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

