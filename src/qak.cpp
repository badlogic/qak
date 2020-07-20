#include "qak.h"
#include "io.h"
#include "parser.h"

#ifdef WASM
#include <emscripten/emscripten.h>
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

EMSCRIPTEN_KEEPALIVE qak_compiler *qak_compiler_new() {
    HeapAllocator *mem = new HeapAllocator();
    Compiler *compiler = mem->allocObject<Compiler>(QAK_SRC_LOC, mem);
    return (qak_compiler *) compiler;
}

EMSCRIPTEN_KEEPALIVE void qak_compiler_delete(qak_compiler *compilerHandle) {
    Compiler *compiler = (Compiler *) compilerHandle;
    HeapAllocator *mem = compiler->mem;
    mem->freeObject(compiler, QAK_SRC_LOC);
    mem->printAllocations();
    delete mem;
}

EMSCRIPTEN_KEEPALIVE void qak_compiler_print_memory_usage(qak_compiler *compilerHandle) {
    Compiler *compiler = (Compiler *) compilerHandle;
    compiler->mem->printAllocations();
}

qak_module *qak_compile(Compiler *compiler, Source *source) {
    BumpAllocator *bumpMem = compiler->mem->allocObject<BumpAllocator>(QAK_SRC_LOC, *compiler->mem);
    Array<Token> tokens(*compiler->mem);
    Errors errors(*compiler->mem, *bumpMem);

    qak::tokenizer::tokenize(*source, tokens, errors);
    if (errors.hasErrors()) {
        return (qak_module *) compiler->mem->allocObject<Module>(QAK_SRC_LOC, *compiler->mem, bumpMem, source, tokens, nullptr, errors);;
    }

    qak::Parser parser(*compiler->mem);
    ast::Module *astModule = parser.parse(*source, errors, bumpMem);
    if (astModule == nullptr) {
        return (qak_module *) compiler->mem->allocObject<Module>(QAK_SRC_LOC, *compiler->mem, bumpMem, source, tokens, nullptr, errors);;
    }

    return (qak_module *) compiler->mem->allocObject<Module>(QAK_SRC_LOC, *compiler->mem, bumpMem, source, tokens, astModule, errors);
}

EMSCRIPTEN_KEEPALIVE qak_module *qak_compiler_compile_file(qak_compiler *compilerHandle, const char *fileName) {
    Compiler *compiler = (Compiler *) compilerHandle;
    Source *source = io::readFile(fileName, *compiler->mem);
    if (source == nullptr) return nullptr;
    return qak_compile(compiler, source);
}

EMSCRIPTEN_KEEPALIVE qak_module *qak_compiler_compile_source(qak_compiler *compilerHandle, const char *fileName, const char *sourceData) {
    Compiler *compiler = (Compiler *) compilerHandle;
    HeapAllocator &mem = *compiler->mem;

    Source *source = Source::fromMemory(mem, fileName, sourceData);
    if (source == nullptr) return nullptr;

    return qak_compile(compiler, source);
}

EMSCRIPTEN_KEEPALIVE void qak_module_delete(qak_module *moduleHandle) {
    Module *module = (Module *) moduleHandle;
    HeapAllocator &mem = module->mem;
    mem.freeObject(module, QAK_SRC_LOC);
}

EMSCRIPTEN_KEEPALIVE void qak_module_get_source(qak_module *moduleHandle, qak_source *source) {
    Module *module = (Module *) moduleHandle;
    source->fileName.data = module->source->fileName;
    source->fileName.length = strlen(module->source->fileName);
    source->data.data = (const char *) module->source->data;
    source->data.length = module->source->size;
}

EMSCRIPTEN_KEEPALIVE int qak_module_get_num_errors(qak_module *moduleHandle) {
    Module *module = (Module *) moduleHandle;
    return (int) module->errors.getErrors().size();
}

EMSCRIPTEN_KEEPALIVE void qak_module_get_error(qak_module *moduleHandle, int errorIndex, qak_error *errorResult) {
    Module *module = (Module *) moduleHandle;
    Error &error = module->errors.getErrors()[errorIndex];
    Span &span = error.span;

    errorResult->errorMessage.data = error.message;
    errorResult->errorMessage.length = strlen(error.message);
    errorResult->span.data.data = (const char *) module->source->data + span.start;
    errorResult->span.data.length = span.end - span.start;
    errorResult->span.start = span.start;
    errorResult->span.end = span.end;
    errorResult->span.startLine = span.startLine;
    errorResult->span.endLine = span.endLine;
}

EMSCRIPTEN_KEEPALIVE int qak_module_get_num_tokens(qak_module *moduleHandle) {
    Module *module = (Module *) moduleHandle;
    return (int) module->tokens.size();
}

EMSCRIPTEN_KEEPALIVE void qak_module_get_token(qak_module *moduleHandle, int tokenIndex, qak_token *tokenResult) {
    Module *module = (Module *) moduleHandle;
    Token &token = module->tokens[tokenIndex];
    Source &source = token.source;

    tokenResult->type = (qak_token_type) token.type;
    tokenResult->span.data.data = (const char *) source.data + token.start;
    tokenResult->span.data.length = token.end - token.start;
    tokenResult->span.start = token.start;
    tokenResult->span.end = token.end;
    tokenResult->span.startLine = token.startLine;
    tokenResult->span.endLine = token.endLine;
}

EMSCRIPTEN_KEEPALIVE void qak_module_print_errors(qak_module *moduleHandle) {
    Module *module = (Module *) moduleHandle;
    module->errors.print();
}

EMSCRIPTEN_KEEPALIVE void qak_module_print_tokens(qak_module *moduleHandle) {
    Module *module = (Module *) moduleHandle;
    HeapAllocator mem;
    tokenizer::printTokens(module->tokens, mem);
}

EMSCRIPTEN_KEEPALIVE void qak_module_print_ast(qak_module *moduleHandle) {
    Module *module = (Module *) moduleHandle;
    if (module->astModule) {
        HeapAllocator mem;
        parser::printAstNode(module->astModule, mem);
    }
}

#ifdef WASM
EMSCRIPTEN_KEEPALIVE int main(int argc, char** argv) {
    return 0;
}

#define member_size(type, member) sizeof(((type *)0)->member)

EMSCRIPTEN_KEEPALIVE void qak_print_struct_offsets () {
    printf("qak_string (size=%lu)\n", sizeof(qak_string));
    printf("   data: %lu\n", offsetof(qak_string, data));
    printf("   length: %lu\n", offsetof(qak_string, length));

    printf("qak_source (size=%lu)\n", sizeof(qak_source));
    printf("   data: %lu\n", offsetof(qak_source, data));
    printf("   fileName: %lu\n", offsetof(qak_source, fileName));

    printf("qak_span (size=%lu)\n", sizeof(qak_span));
    printf("   data: %lu\n", offsetof(qak_span, data));
    printf("   start: %lu\n", offsetof(qak_span, start));
    printf("   end: %lu\n", offsetof(qak_span, end));
    printf("   startLine: %lu\n", offsetof(qak_span, startLine));
    printf("   endLine: %lu\n", offsetof(qak_span, endLine));

    printf("qak_line (size=%lu)\n", sizeof(qak_line));
    printf("   source: %lu\n", offsetof(qak_line, source));
    printf("   data: %lu\n", offsetof(qak_line, data));
    printf("   lineNumber: %lu\n", offsetof(qak_line, lineNumber));

    printf("qak_token (size=%lu)\n", sizeof(qak_token));
    printf("   type(=%lu): %lu\n", member_size(qak_token, type), offsetof(qak_token, type));
    printf("   span: %lu\n", offsetof(qak_token, span));

    printf("qak_error (size=%lu)\n", sizeof(qak_error));
    printf("   errorMessage: %lu\n", offsetof(qak_error, errorMessage));
    printf("   span: %lu\n", offsetof(qak_error, span));
}
#endif