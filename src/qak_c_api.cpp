#include "qak.h"
#include "qak_c_api.h"
#include "parser.h"

using namespace qak;

struct Compiler {
    HeapAllocator *mem;
    Errors errors;

    Compiler(HeapAllocator *mem) : mem(mem), errors(*mem) {};
};

struct Module {
    HeapAllocator &mem;
    Source source;
    Array<Token> tokens;
    ast::Module *astModule;

    Module(HeapAllocator &mem, Source source, Array<Token> &tokens, ast::Module *astModule) : mem(mem), source(source), tokens(mem), astModule(astModule) {
        this->tokens.addAll(tokens);
    };

    ~Module() {
        source.buffer.free();
        mem.freeObject(astModule, __FILE__, __LINE__);
    }
};

qak_compiler qak_compiler_new() {
    HeapAllocator *mem = new HeapAllocator();
    Compiler *compiler = mem->allocObject<Compiler>(__FILE__, __LINE__, mem);
    return compiler;
}

void qak_compiler_delete(qak_compiler compilerHandle) {
    Compiler *compiler = (Compiler *) compilerHandle;
    HeapAllocator *mem = compiler->mem;
    mem->freeObject(compiler, __FILE__, __LINE__);
    mem->printAllocations();
    delete mem;
}

qak_module qak_compile_file(qak_compiler compilerHandle, const char *fileName) {
    Compiler *compiler = (Compiler *) compilerHandle;
    Buffer buffer = io::readFile(fileName, *compiler->mem);
    if (buffer.data == nullptr) return nullptr;
    Source source = {buffer, fileName};

    Array<Token> tokens(*compiler->mem);
    qak::tokenizer::tokenize(source, tokens, compiler->errors);
    if (compiler->errors.hasErrors()) return nullptr;

    qak::Parser parser(*compiler->mem);
    ast::Module *astModule = parser.parse(source, compiler->errors);
    if (astModule == nullptr) return nullptr;

    Module *module = compiler->mem->allocObject<Module>(__FILE__, __LINE__, *compiler->mem, source, tokens, astModule);
    return module;
}

void qak_module_delete(qak_module moduleHandle) {
    Module *module = (Module *) moduleHandle;
    HeapAllocator &mem = module->mem;
    mem.freeObject(module, __FILE__, __LINE__);
}