#ifndef BACKEND_H_
#define BACKEND_H_ 1

struct AST_Node;
struct Compiler;

typedef struct CompilerBackend {
    void (*init)(Compiler*);
    void (*generate)(Compiler*, AST_Node*);
} CompilerBackend;

extern void LoadBackend_ANM(CompilerBackend* self);
extern void LoadBackend_ECL(CompilerBackend* self);

#endif /* BACKEND_H_ */
