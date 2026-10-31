#include "circularis.h"
#include "lexer.cpp"
#include "parser.cpp"
#include "compiler.cpp"

Compiler* gCompiler = NULL;

#define MULTILINE_STR(s) #s

const char* sourceCode = MULTILINE_STR(
    int gregery = (5 + 4*2);

    int gregorySpeak();
    float definedFunction() {}
    void main(int g, float x, string str, int e) {
        return (g + 6 * 7 / (x - 10) * (((e * 4 + 1))));
    }
);

int CompilerTest(void)
{
    Compiler compiler;
    gCompiler = &compiler;

    compiler.init(&compiler, sourceCode);
    compiler.doLex(&compiler);
    compiler.doParse(&compiler);

    compiler.printInfo(&compiler);

    return 0;
}

int main(int argc, const char* argv[])
{
    CompilerTest();
    return 0;
}
