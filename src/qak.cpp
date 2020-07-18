#include "qak.h"
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

    Compiler(BumpAllocator *bumpMem, HeapAllocator *mem) : bumpMem(bumpMem), mem(mem) {};
};

// BOZO each module should get its own HeapAllocator and BumpAllocator. Destructing a module
// should kill the memory of the AST, source buffer, etc. and the allocators themselves.
struct Module {
    HeapAllocator &mem;
    Source *source;
    Array<Token> tokens;
    ast::Module *astModule;
    Errors errors;

    Module(HeapAllocator &mem, Source *source, Array<Token> &tokens, ast::Module *astModule, Errors &errors) : mem(mem), source(source), tokens(mem), astModule(astModule), errors(mem) {
        this->tokens.addAll(tokens);
        this->errors.addAll(errors);
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
    printf("Created compiler\n");
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

qak_module qak_compile(Compiler* compiler, Source *source) {
    Array<Token> tokens(*compiler->mem);
    Errors errors(*compiler->mem);
    qak::tokenizer::tokenize(*source, tokens, errors);
    if (errors.hasErrors()) {
        return compiler->mem->allocObject<Module>(QAK_SRC_LOC, *compiler->mem, source, tokens, nullptr, errors);;
    }

    qak::Parser parser(*compiler->mem);
    ast::Module *astModule = parser.parse(*source, errors, compiler->bumpMem);
    if (astModule == nullptr) {
        return compiler->mem->allocObject<Module>(QAK_SRC_LOC, *compiler->mem, source, tokens, nullptr, errors);;
    }

    return compiler->mem->allocObject<Module>(QAK_SRC_LOC, *compiler->mem, source, tokens, astModule, errors);
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

    size_t dataLength = strlen(sourceData);
    uint8_t *data = mem.alloc<uint8_t>(dataLength, QAK_SRC_LOC);
    memcpy(data, sourceData, dataLength);

    size_t fileNameLength = strlen(fileName);
    char* fileNameCopy = mem.alloc<char>(fileNameLength, QAK_SRC_LOC);
    memcpy(fileNameCopy, fileName, fileNameLength);

    Source *source = mem.allocObject<Source>(QAK_SRC_LOC, mem, fileNameCopy, data, dataLength);
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
    tokenizer::printTokens(module->tokens, module->mem);
}

EMSCRIPTEN_KEEPALIVE void qak_module_print_ast(qak_module moduleHandle) {
    Module *module = (Module *) moduleHandle;
    if (module->astModule) {
        parser::printAstNode(module->astModule, module->mem);
    }
}

EMSCRIPTEN_KEEPALIVE int qak_version() {
    printf("Hello world.\n");
	return 123;
}

// BOZO linker complains otherwise, dunno why.
#ifdef WASM
EMSCRIPTEN_KEEPALIVE int main(int argc, char** argv) {
	return 0;
}
#endif