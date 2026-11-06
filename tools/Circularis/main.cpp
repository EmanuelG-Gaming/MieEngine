#include "circularis.h"
#include "lexer.cpp"
#include "parser.cpp"
#include "semantics.cpp"
#include "compiler.cpp"

Compiler* gCompiler = NULL;

#define MULTILINE_STR(s) #s

const char* sourceCode = MULTILINE_STR(
    int as(int x, float y)
    {
        return x + 4;
    }

    void main() {
        as(5 + 2, 5+1-35);
    }
);

/*
const char* sourceCode = MULTILINE_STR(
    void main()
    {
    }
);
*/


int CompilerTest(void)
{
    Compiler* compiler = (Compiler *) _CALLOC(1, sizeof(Compiler));
    assert(compiler != NULL);

    gCompiler = compiler;

    compiler->init(compiler, sourceCode);
    compiler->doLex(compiler);
    compiler->doParse(compiler);
    compiler->doValidation(compiler);

    compiler->printInfo(compiler);

    _FREE(compiler);

    return 0;
}

int main(int argc, const char* argv[])
{
    CompilerTest();
    return 0;
}
