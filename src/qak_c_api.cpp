#include "qak_c_api.h"
#include "io.h"
#include "parser.h"

#ifdef WASM
#include "emscripten.h"
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

using namespace qak;

struct Compiler {
    BumpAllocator *bumpMem;
    HeapAllocator *mem;
    Errors errors;

    Compiler(BumpAllocator *bumpMem, HeapAllocator *mem) : bumpMem(bumpMem), mem(mem), errors(*mem) {};
};

// BOZO each module should get its own HeapAllocator and BumpAllocator. Destructing a module
// should kill the memory of the AST, source buffer, etc. and the allocators themselves.
struct Module {
    HeapAllocator &mem;
    Source *source;
    Array<Token> tokens;
    ast::Module *astModule;

    Module(HeapAllocator &mem, Source *source, Array<Token> &tokens, ast::Module *astModule) : mem(mem), source(source), tokens(mem), astModule(astModule) {
        this->tokens.addAll(tokens);
    };

    ~Module() {
        mem.freeObject(source, QAK_SRC_LOC);

        // BOZO also need to tie one BumpAllocator to one Module
        // and clean it up here.
    }
};

EMSCRIPTEN_KEEPALIVE qak_compiler qak_compiler_new() {
    HeapAllocator *mem = new HeapAllocator();
    BumpAllocator *bumpMem = new BumpAllocator();
    Compiler *compiler = mem->allocObject<Compiler>(QAK_SRC_LOC, bumpMem, mem);
    return compiler;
}

EMSCRIPTEN_KEEPALIVE void qak_compiler_delete(qak_compiler compilerHandle) {
    Compiler *compiler = (Compiler *) compilerHandle;
    BumpAllocator *bumpMem = compiler->bumpMem;
    HeapAllocator *mem = compiler->mem;
    mem->freeObject(compiler, QAK_SRC_LOC);
    mem->printAllocations();
    delete mem;
    delete bumpMem;
}

EMSCRIPTEN_KEEPALIVE qak_module qak_compile_file(qak_compiler compilerHandle, const char *fileName) {
    Compiler *compiler = (Compiler *) compilerHandle;
    Source *source = io::readFile(fileName, *compiler->mem);
    if (source == nullptr) return nullptr;

    Array<Token> tokens(*compiler->mem);
    qak::tokenizer::tokenize(*source, tokens, compiler->errors);
    if (compiler->errors.hasErrors()) return nullptr;

    qak::Parser parser(*compiler->mem);
    ast::Module *astModule = parser.parse(*source, compiler->errors, compiler->bumpMem);
    if (astModule == nullptr) return nullptr;

    Module *module = compiler->mem->allocObject<Module>(QAK_SRC_LOC, *compiler->mem, source, tokens, astModule);
    return module;
}

EMSCRIPTEN_KEEPALIVE void qak_module_delete(qak_module moduleHandle) {
    Module *module = (Module *) moduleHandle;
    HeapAllocator &mem = module->mem;
    mem.freeObject(module, QAK_SRC_LOC);
}

EMSCRIPTEN_KEEPALIVE int qak_version() {
	return 123;
}

// BOZO linker complains otherwise, dunno why.
#ifdef WASM
EMSCRIPTEN_KEEPALIVE int main(int argc, char** argv) {
	return 0;
}
#endif