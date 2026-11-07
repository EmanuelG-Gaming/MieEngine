#include "circularis.h"
#include "lexer.cpp"
#include "parser.cpp"
#include "semantics.cpp"
#include "compiler.cpp"

#include "../../ext/file_lib.h"
#include "../../ext/file_lib.cpp"

Compiler* gCompiler = NULL;

#define MULTILINE_STR(s) #s

const char* sourceCode = MULTILINE_STR(
    int as(int x, float y)
    {
        if floor(x) + 1 {
            return 3;
        }

        return callSomething(x, x + y % 5);
    }

    int main()
    {
        int g = as(5, -4.0f);
        return g;
    }
);

int CompilerTest(const char* sourceFileName)
{
    void* dataBuf = NULL;
    int size;
    LoadFileMemRef(sourceFileName, &dataBuf, &size);
    if (dataBuf == NULL)
    {
        fprintf(stderr, "Compiler error: File %s not found!\n", sourceFileName);
        return -1;
    }


    Compiler* compiler = (Compiler *) _CALLOC(1, sizeof(Compiler));
    assert(compiler != NULL);

    gCompiler = compiler;

    compiler->init(compiler, static_cast<char *> (dataBuf), size);
    compiler->doLex(compiler);

    compiler->scanner.print(&compiler->scanner);

    compiler->doParse(compiler);
    compiler->doValidation(compiler);

    compiler->printInfo(compiler);

    _FREE(compiler);

    return 0;
}

int main(int argc, const char* argv[])
{
    if (argc == 2)
    {
        const char* inPath = argv[1];
        CompilerTest(inPath);
        return 0;
    }
    else if (argc == 1)
    {
        fprintf(stderr, "USAGE: 'crl sourceFile'");
        return -1;
    }
    else
    {
        fprintf(stderr, "USAGE: 'crl sourceFile'");
        return -1;
    }
    return 0;
}
