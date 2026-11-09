#ifndef BACKEND_H_
#define BACKEND_H_ 1

typedef struct CompilerBackend {
    void (*generate)(void*);
} CompilerBackend;

#endif /* BACKEND_H_ */
