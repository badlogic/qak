#include "c/qak.h"
#include "c/io.h"
#include "c/parser.h"
#include "c/tokenizer.h"
#include "c/error.h"
#include "test.h"
#include <string.h>

void testModule() {
    printf("========= Test: parser simple module\n");
    qak_allocator mem = qak_heap_allocator_init();

    qak_source *source = qak_io_read_source_from_file(&mem, "data/parser_module.qak");
    QAK_CHECK(source, "Couldn't read test file data/parser_module.qak");

    qak_array_token *tokens = qak_array_token_new(&mem, 16);
    qak_errors errors = qak_errors_init(&mem);
    qak_allocator bump = qak_bump_allocator_init(sizeof(qak_ast_node) * 16);
    qak_ast_node *module = qak_parse(source, tokens, &errors, &bump);
    QAK_CHECK(module, "Expected module, got null pointer.");

    qak_allocator_shutdown(&bump);
    qak_allocator_shutdown(&mem);
    printf("SUCCESS\n");
}

void testFunction() {
    printf("========= Test: parse function\n");
    qak_allocator mem = qak_heap_allocator_init();

    {
        qak_source *source = qak_io_read_source_from_memory(&mem, "function.qak", "module test\nfunction foo()\nend");
        qak_errors errors = qak_errors_init(&mem);
        qak_array_token *tokens = qak_array_token_new(&mem, 16);
        qak_allocator bump = qak_bump_allocator_init(sizeof(qak_ast_node) * 16);
        qak_ast_node *module = qak_parse(source, tokens, &errors, &bump);
        qak_errors_print(&errors);
        QAK_CHECK(module, "Expected module, got null pointer.");

        QAK_CHECK(module->data.module.numFunctions == 1, "Expected 1 function.");
        qak_ast_node* function = module->data.module.functions;
        QAK_CHECK(function->next == NULL, "Expected 1 function.");
        QAK_CHECK(qak_span_matches(&function->data.function.name, QAK_STR("foo")), "Expected function name 'foo'.");
        QAK_CHECK(function->data.function.parameters == NULL, "Expected 0 parameters.");
        QAK_CHECK(function->data.function.numParameters == 0, "Expected 0 parameters.");
        QAK_CHECK(function->data.function.returnType == NULL, "Expected no return type.");
        QAK_CHECK(function->data.function.statements == NULL, "Expected 0 statements.");
        QAK_CHECK(function->data.function.numStatements == 0, "Expected 0 statements.");

        qak_source_delete(source);
        qak_errors_shutdown(&errors);
        qak_array_token_delete(tokens);
        qak_allocator_shutdown(&bump);
        qak_allocator_print(&mem);
        QAK_CHECK(qak_allocator_num_allocated_bytes(&mem) == 0, "Expected no allocated memory.");
    }

    {
        qak_source *source = qak_io_read_source_from_memory(&mem, "function.qak", "module test\nfunction foo(): int32\nend");
        qak_errors errors = qak_errors_init(&mem);
        qak_array_token *tokens = qak_array_token_new(&mem, 16);
        qak_allocator bump = qak_bump_allocator_init(sizeof(qak_ast_node) * 16);
        qak_ast_node *module = qak_parse(source, tokens, &errors, &bump);
        qak_errors_print(&errors);
        QAK_CHECK(module, "Expected module, got null pointer.");

        QAK_CHECK(module->data.module.numFunctions == 1, "Expected 1 function.");
        qak_ast_node* function = module->data.module.functions;
        QAK_CHECK(function->next == NULL, "Expected 1 function.");
        QAK_CHECK(qak_span_matches(&function->data.function.name, QAK_STR("foo")), "Expected function name 'foo'.");
        QAK_CHECK(function->data.function.parameters == NULL, "Expected 0 parameters.");
        QAK_CHECK(function->data.function.numParameters == 0, "Expected 0 parameters.");
        QAK_CHECK(function->data.function.statements == NULL, "Expected 0 statements.");
        QAK_CHECK(function->data.function.numStatements == 0, "Expected 0 statements.");

        QAK_CHECK(function->data.function.returnType, "Expected return type.");
        QAK_CHECK(qak_span_matches(&function->data.function.returnType->data.typeSpecifier.name, QAK_STR("int32")), "Expected int32 return type.");

        qak_source_delete(source);
        qak_errors_shutdown(&errors);
        qak_array_token_delete(tokens);
        qak_allocator_shutdown(&bump);
        qak_allocator_print(&mem);
        QAK_CHECK(qak_allocator_num_allocated_bytes(&mem) == 0, "Expected no allocated memory.");
    }

    {
        qak_source *source = qak_io_read_source_from_memory(&mem, "function.qak", "module test\nfunction foo(a: int32): int32\nend");
        qak_errors errors = qak_errors_init(&mem);
        qak_array_token *tokens = qak_array_token_new(&mem, 16);
        qak_allocator bump = qak_bump_allocator_init(sizeof(qak_ast_node) * 16);
        qak_ast_node *module = qak_parse(source, tokens, &errors, &bump);
        qak_errors_print(&errors);
        QAK_CHECK(module, "Expected module, got null pointer.");

        QAK_CHECK(module->data.module.numFunctions == 1, "Expected 1 function.");
        qak_ast_node* function = module->data.module.functions;
        QAK_CHECK(function->next == NULL, "Expected 1 function.");
        QAK_CHECK(qak_span_matches(&function->data.function.name, QAK_STR("foo")), "Expected function name 'foo'.");
        QAK_CHECK(function->data.function.statements == NULL, "Expected 0 statements.");
        QAK_CHECK(function->data.function.numStatements == 0, "Expected 0 statements.");

        QAK_CHECK(function->data.function.returnType, "Expected return type.");
        QAK_CHECK(qak_span_matches(&function->data.function.returnType->data.typeSpecifier.name, QAK_STR("int32")), "Expected int32 return type.");

        QAK_CHECK(function->data.function.parameters, "Expected parameters.");
        QAK_CHECK(function->data.function.numParameters == 1, "Expected 1 parameter.");
        qak_ast_node *parameter = function->data.function.parameters;
        QAK_CHECK(parameter->next == NULL, "Expected 1 parameter.");
        QAK_CHECK(qak_span_matches(&parameter->data.parameter.name, QAK_STR("a")), "Expected parameter a.");
        QAK_CHECK(qak_span_matches(&parameter->data.parameter.typeSpecifier->data.typeSpecifier.name, QAK_STR("int32")), "Expected parameter a.");

        qak_source_delete(source);
        qak_errors_shutdown(&errors);
        qak_array_token_delete(tokens);
        qak_allocator_shutdown(&bump);
        qak_allocator_print(&mem);
        QAK_CHECK(qak_allocator_num_allocated_bytes(&mem) == 0, "Expected no allocated memory.");
    }

    {
        qak_source *source = qak_io_read_source_from_memory(&mem, "function.qak", "module test\nfunction foo(a: int32, b: float, c: double, d: int32): int32\nend");
        qak_errors errors = qak_errors_init(&mem);
        qak_array_token *tokens = qak_array_token_new(&mem, 16);
        qak_allocator bump = qak_bump_allocator_init(sizeof(qak_ast_node) * 16);
        qak_ast_node *module = qak_parse(source, tokens, &errors, &bump);
        qak_errors_print(&errors);
        QAK_CHECK(module, "Expected module, got null pointer.");

        QAK_CHECK(module->data.module.numFunctions == 1, "Expected 1 function.");
        qak_ast_node* function = module->data.module.functions;
        QAK_CHECK(function->next == NULL, "Expected 1 function.");
        QAK_CHECK(qak_span_matches(&function->data.function.name, QAK_STR("foo")), "Expected function name 'foo'.");
        QAK_CHECK(function->data.function.statements == NULL, "Expected 0 statements.");
        QAK_CHECK(function->data.function.numStatements == 0, "Expected 0 statements.");

        QAK_CHECK(function->data.function.returnType, "Expected return type.");
        QAK_CHECK(qak_span_matches(&function->data.function.returnType->data.typeSpecifier.name, QAK_STR("int32")), "Expected int32 return type.");

        QAK_CHECK(function->data.function.parameters, "Expected parameters.");
        QAK_CHECK(function->data.function.numParameters == 4, "Expected 4 parameter.");
        qak_ast_node *parameter = function->data.function.parameters;
        QAK_CHECK(parameter->next, "Expected more parameters.");
        QAK_CHECK(qak_span_matches(&parameter->data.parameter.name, QAK_STR("a")), "Expected parameter a.");
        QAK_CHECK(qak_span_matches(&parameter->data.parameter.typeSpecifier->data.typeSpecifier.name, QAK_STR("int32")), "Expected type int32.");
        parameter = parameter->next;

        QAK_CHECK(parameter->next, "Expected more parameters.");
        QAK_CHECK(qak_span_matches(&parameter->data.parameter.name, QAK_STR("b")), "Expected parameter b.");
        QAK_CHECK(qak_span_matches(&parameter->data.parameter.typeSpecifier->data.typeSpecifier.name, QAK_STR("float")), "Expected type float.");
        parameter = parameter->next;

        QAK_CHECK(parameter->next, "Expected more parameters.");
        QAK_CHECK(qak_span_matches(&parameter->data.parameter.name, QAK_STR("c")), "Expected parameter c.");
        QAK_CHECK(qak_span_matches(&parameter->data.parameter.typeSpecifier->data.typeSpecifier.name, QAK_STR("double")), "Expected type double.");
        parameter = parameter->next;

        QAK_CHECK(parameter->next == NULL, "Expected no more parameters.");
        QAK_CHECK(qak_span_matches(&parameter->data.parameter.name, QAK_STR("d")), "Expected parameter d.");
        QAK_CHECK(qak_span_matches(&parameter->data.parameter.typeSpecifier->data.typeSpecifier.name, QAK_STR("int32")), "Expected type int32.");
        parameter = parameter->next;

        qak_source_delete(source);
        qak_errors_shutdown(&errors);
        qak_array_token_delete(tokens);
        qak_allocator_shutdown(&bump);
        qak_allocator_print(&mem);
        QAK_CHECK(qak_allocator_num_allocated_bytes(&mem) == 0, "Expected no allocated memory.");
    }

    qak_allocator_print(&mem);
    QAK_CHECK(qak_allocator_num_allocated_bytes(&mem) == 0, "Parser shutdown didn't deallocate all memory.");

    qak_allocator_shutdown(&mem);
    printf("SUCCESS\n");
}

void testVariable() {
    printf("========= Test: parser varriable\n");
    qak_allocator mem = qak_heap_allocator_init();

    {
        qak_source *source = qak_io_read_source_from_memory(&mem, "function.qak", "module test\nvar a");
        qak_errors errors = qak_errors_init(&mem);
        qak_array_token *tokens = qak_array_token_new(&mem, 16);
        qak_allocator bump = qak_bump_allocator_init(sizeof(qak_ast_node) * 16);
        qak_ast_node *module = qak_parse(source, tokens, &errors, &bump);
        qak_errors_print(&errors);
        QAK_CHECK(module, "Expected module, got null pointer.");

        QAK_CHECK(module->data.module.numStatements == 1, "Expected 1 statement.");
        qak_ast_node *variable = module->data.module.statements;
        QAK_CHECK(variable->type == QakAstVariable, "Expected variable AST node.");
        QAK_CHECK(qak_span_matches(&variable->data.variable.name, QAK_STR("a")), "Expected variable a.");
        QAK_CHECK(variable->data.variable.typeSpecifier == NULL, "Expected no type specifier.");
        QAK_CHECK(variable->data.variable.initializerExpression == NULL, "Expected no initializer.");

        qak_source_delete(source);
        qak_errors_shutdown(&errors);
        qak_array_token_delete(tokens);
        qak_allocator_shutdown(&bump);
        qak_allocator_print(&mem);
        QAK_CHECK(qak_allocator_num_allocated_bytes(&mem) == 0, "Expected no allocated memory.");
    }

    {
        qak_source *source = qak_io_read_source_from_memory(&mem, "function.qak", "module test\nvar a: int32");
        qak_errors errors = qak_errors_init(&mem);
        qak_array_token *tokens = qak_array_token_new(&mem, 16);
        qak_allocator bump = qak_bump_allocator_init(sizeof(qak_ast_node) * 16);
        qak_ast_node *module = qak_parse(source, tokens, &errors, &bump);
        qak_errors_print(&errors);
        QAK_CHECK(module, "Expected module, got null pointer.");

        QAK_CHECK(module->data.module.numStatements == 1, "Expected 1 statement.");
        qak_ast_node *variable = module->data.module.statements;
        QAK_CHECK(variable->type == QakAstVariable, "Expected variable AST node.");
        QAK_CHECK(qak_span_matches(&variable->data.variable.name, QAK_STR("a")), "Expected variable a.");
        QAK_CHECK(variable->data.variable.typeSpecifier, "Expected type specifier.");
        QAK_CHECK(qak_span_matches(&variable->data.variable.typeSpecifier->data.typeSpecifier.name, QAK_STR("int32")), "Expected int32 type.");
        QAK_CHECK(variable->data.variable.initializerExpression == NULL, "Expected no initializer.");

        qak_source_delete(source);
        qak_errors_shutdown(&errors);
        qak_array_token_delete(tokens);
        qak_allocator_shutdown(&bump);
        qak_allocator_print(&mem);
        QAK_CHECK(qak_allocator_num_allocated_bytes(&mem) == 0, "Expected no allocated memory.");
    }

    qak_allocator_print(&mem);
    QAK_CHECK(qak_allocator_num_allocated_bytes(&mem) == 0, "Parser shutdown didn't deallocate all memory.");

    qak_allocator_shutdown(&mem);
    printf("SUCCESS\n");
}

void testError() {
    printf("========= Test: parser test error expect\n");
    qak_allocator mem = qak_heap_allocator_init();

    {
        qak_errors errors = qak_errors_init(&mem);
        qak_source *source = qak_io_read_source_from_memory(&mem, "error.qak", "");
        qak_array_token *tokens = qak_array_token_new(&mem, 16);
        qak_allocator bump = qak_bump_allocator_init(sizeof(qak_ast_node) * 16);
        qak_parse(source, tokens, &errors, &bump);
        QAK_CHECK(errors.errors->size == 1, "Expected an error, got no error.");
        qak_error_print(&errors.errors->items[0]);
        qak_errors_shutdown(&errors);
        qak_array_token_delete(tokens);
        qak_allocator_shutdown(&bump);
        qak_source_delete(source);
    }

    {
        qak_errors errors = qak_errors_init(&mem);
        qak_source *source = qak_io_read_source_from_memory(&mem, "error.qak", "  \n\t\n");
        qak_array_token *tokens = qak_array_token_new(&mem, 16);
        qak_allocator bump = qak_bump_allocator_init(sizeof(qak_ast_node) * 16);
        qak_parse(source, tokens, &errors, &bump);
        QAK_CHECK(errors.errors->size == 1, "Expected an error, got no error.");
        qak_error_print(&errors.errors->items[0]);
        qak_errors_shutdown(&errors);
        qak_array_token_delete(tokens);
        qak_allocator_shutdown(&bump);
        qak_source_delete(source);
    }

    {
        qak_errors errors = qak_errors_init(&mem);
        qak_source *source = qak_io_read_source_from_memory(&mem, "error.qak", "123");
        qak_array_token *tokens = qak_array_token_new(&mem, 16);
        qak_allocator bump = qak_bump_allocator_init(sizeof(qak_ast_node) * 16);
        qak_parse(source, tokens, &errors, &bump);
        QAK_CHECK(errors.errors->size == 1, "Expected an error, got no error.");
        qak_error_print(&errors.errors->items[0]);
        qak_errors_shutdown(&errors);
        qak_array_token_delete(tokens);
        qak_allocator_shutdown(&bump);
        qak_source_delete(source);
    }

    {
        qak_errors errors = qak_errors_init(&mem);
        qak_source *source = qak_io_read_source_from_memory(&mem, "error.qak", "module");
        qak_array_token *tokens = qak_array_token_new(&mem, 16);
        qak_allocator bump = qak_bump_allocator_init(sizeof(qak_ast_node) * 16);
        qak_parse(source, tokens, &errors, &bump);
        QAK_CHECK(errors.errors->size == 1, "Expected an error, got no error.");
        qak_error_print(&errors.errors->items[0]);
        qak_errors_shutdown(&errors);
        qak_array_token_delete(tokens);
        qak_allocator_shutdown(&bump);
        qak_source_delete(source);
    }

    qak_allocator_print(&mem);
    QAK_CHECK(qak_allocator_num_allocated_bytes(&mem) == 0, "Parser shutdown didn't deallocate all memory.");

    qak_allocator_shutdown(&mem);
    printf("SUCCESS\n");
}

int main(int argc, char **argv) {
    QAK_UNUSED(argc);
    QAK_UNUSED(argv);

    testError();
    testModule();
    testFunction();
    testVariable();
    return 0;
}
