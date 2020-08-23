#include "qak.h"
#include "io.h"
#include "error.h"
#include "tokenizer.h"
#include "parser.h"

qak_module *compile(qak_allocator mem, qak_source *source) {
    qak_module *module = QAK_ALLOCATE(&mem, qak_module, 1);
    module->source = source;
    module->mem = mem; // transfer ownership of allocator to module
    module->bumpMem = qak_bump_allocator_init(sizeof(qak_ast_node) * 256);
    module->errors = qak_errors_init(&module->mem);
    module->tokens = qak_array_token_new(&module->mem, 16);
    module->astModule = NULL;

    qak_tokenize(source, module->tokens, &module->errors);
    if (module->errors.errors->size) return module;

    module->astModule = qak_parse(source, module->tokens, &module->errors, &module->bumpMem);
    return module;
}

qak_module *qak_compiler_compile_file(const char *fileName) {
    qak_allocator mem = qak_heap_allocator_init();
    qak_source *source = qak_io_read_source_from_file(&mem, fileName);
    if (source == NULL) return NULL;
    return compile(mem, source);
}

qak_module *qak_compiler_compile_source(const char *fileName, const char *sourceCode) {
    qak_allocator mem = qak_heap_allocator_init();
    qak_source *source = qak_io_read_source_from_memory(&mem, fileName, sourceCode);
    if (source == NULL) return NULL;
    return compile(mem, source);
}

void qak_module_delete(qak_module *module) {
    // Delete ast nodes
    qak_allocator_shutdown(&module->bumpMem);

    // Delete everything else, including the module itself
    qak_allocator_shutdown(&module->mem);
}