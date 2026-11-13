#include "backend.h"
#include "../circularis.h"

static void init(Compiler* compiler)
{
}
static void generate(Compiler* compiler, AST_Node* parent)
{
}

extern void LoadBackend_ANM(CompilerBackend *self)
{
    self->init = init;
    self->generate = generate;
}

