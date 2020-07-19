#include "qak.h"
#include "io.h"
#include "parser.h"

#ifdef WASM
#include "emscripten.h"
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

using namespace qak;

/** Keeps track of global memory allocated for Sources and Modules via a HeapAllocator **/
struct Compiler {
    HeapAllocator *mem;

    Compiler(HeapAllocator *mem) : mem(mem) {};
};

/** Keeps track of results from all compilation stages for a module. The module itself
 * is memory managed by a Compiler. The data held by the module is memory managed by
 * a BumpAllocator owned by the module. The module has ownership of the Source it was
 * compiled from and frees it upon desctruction. */
struct Module {
    HeapAllocator &mem;
    BumpAllocator *bumpMem;
    Source *source;
    Array<Token> tokens;
    ast::Module *astModule;
    Errors errors;

    Module(HeapAllocator &mem, BumpAllocator *bumpMem, Source *source, Array<Token> &tokens, ast::Module *astModule, Errors &errors) :
        mem(mem), bumpMem(bumpMem),
        source(source),
        tokens(mem),
        astModule(astModule),
        errors(mem, *bumpMem) {
        this->tokens.addAll(tokens);
        this->errors.addAll(errors);
    };

    ~Module() {
        // Free all objects allocated through the bump allocator such
        // as the AST.
        if (bumpMem) mem.freeObject(bumpMem, QAK_SRC_LOC);

        // Free all objects allocated through the heap allocator such
        // as the source.
        mem.freeObject(source, QAK_SRC_LOC);
    }
};

EMSCRIPTEN_KEEPALIVE qak_compiler qak_compiler_new() {
    HeapAllocator *mem = new HeapAllocator();
    Compiler *compiler = mem->allocObject<Compiler>(QAK_SRC_LOC, mem);
    return compiler;
}

EMSCRIPTEN_KEEPALIVE void qak_compiler_delete(qak_compiler compilerHandle) {
    Compiler *compiler = (Compiler *) compilerHandle;
    HeapAllocator *mem = compiler->mem;
    mem->freeObject(compiler, QAK_SRC_LOC);
    mem->printAllocations();
    delete mem;
}

EMSCRIPTEN_KEEPALIVE void qak_compiler_print_memory_usage(qak_compiler compilerHandle) {
    Compiler *compiler = (Compiler *) compilerHandle;
    compiler->mem->printAllocations();
}

qak_module qak_compile(Compiler* compiler, Source *source) {
    BumpAllocator *bumpMem = compiler->mem->allocObject<BumpAllocator>(QAK_SRC_LOC, *compiler->mem);
    Array<Token> tokens(*compiler->mem);
    Errors errors(*compiler->mem, *bumpMem);

    qak::tokenizer::tokenize(*source, tokens, errors);
    if (errors.hasErrors()) {
        return compiler->mem->allocObject<Module>(QAK_SRC_LOC, *compiler->mem, bumpMem, source, tokens, nullptr, errors);;
    }

    qak::Parser parser(*compiler->mem);
    ast::Module *astModule = parser.parse(*source, errors, bumpMem);
    if (astModule == nullptr) {
        return compiler->mem->allocObject<Module>(QAK_SRC_LOC, *compiler->mem, bumpMem, source, tokens, nullptr, errors);;
    }

    return compiler->mem->allocObject<Module>(QAK_SRC_LOC, *compiler->mem, bumpMem, source, tokens, astModule, errors);
}

EMSCRIPTEN_KEEPALIVE qak_module qak_compile_file(qak_compiler compilerHandle, const char *fileName) {
    Compiler *compiler = (Compiler *) compilerHandle;
    Source *source = io::readFile(fileName, *compiler->mem);
    if (source == nullptr) return nullptr;
    return qak_compile(compiler, source);
}

EMSCRIPTEN_KEEPALIVE qak_module qak_compile_source(qak_compiler compilerHandle, const char *fileName, const char *sourceData) {
    Compiler *compiler = (Compiler *) compilerHandle;
    HeapAllocator &mem = *compiler->mem;

    Source *source = Source::fromMemory(mem, fileName, sourceData);
    if (source == nullptr) return nullptr;

    return qak_compile(compiler, source);
}

EMSCRIPTEN_KEEPALIVE void qak_module_delete(qak_module moduleHandle) {
    Module *module = (Module *) moduleHandle;
    HeapAllocator &mem = module->mem;
    mem.freeObject(module, QAK_SRC_LOC);
}

EMSCRIPTEN_KEEPALIVE int qak_module_has_errors(qak_module moduleHandle) {
    Module *module = (Module *) moduleHandle;
    return (int)module->errors.getErrors().size();
}

EMSCRIPTEN_KEEPALIVE void qak_module_print_errors(qak_module moduleHandle) {
    Module *module = (Module *) moduleHandle;
    module->errors.print();
}

EMSCRIPTEN_KEEPALIVE void qak_module_print_tokens(qak_module moduleHandle) {
    Module *module = (Module *) moduleHandle;
    HeapAllocator mem;
    tokenizer::printTokens(module->tokens, mem);
}

EMSCRIPTEN_KEEPALIVE void qak_module_print_ast(qak_module moduleHandle) {
    Module *module = (Module *) moduleHandle;
    if (module->astModule) {
        HeapAllocator mem;
        parser::printAstNode(module->astModule, mem);
    }
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