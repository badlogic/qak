#include "c/qak.h"
#include "c/io.h"
#include "c/parser.h"
#include "c/tokenizer.h"
#include "c/error.h"
#include "test.h"
#include <string.h>

typedef struct parser_test_data {
    qak_source *source;
    qak_errors errors;
    qak_array_token *tokens;
    qak_allocator bump;
    qak_ast_module *module;
} parser_test_data;

parser_test_data createTestData(qak_allocator *mem, const char *sourceCode) {
    qak_source *source = qak_io_read_source_from_memory(mem, "test.qak", sourceCode);
    qak_errors errors = qak_errors_init(mem);
    qak_array_token *tokens = qak_array_token_new(mem, 16);
    qak_allocator bump = qak_bump_allocator_init(sizeof(qak_ast_node) * 16);
    qak_ast_module *module = qak_parse(source, tokens, &errors, &bump);
    qak_errors_print(&errors);
    return (parser_test_data) {source, errors, tokens, bump, module};
}

void destroyTestData(qak_allocator *mem, parser_test_data data) {
    qak_source_delete(data.source);
    qak_errors_shutdown(&data.errors);
    qak_array_token_delete(data.tokens);
    qak_allocator_shutdown(&data.bump);
    qak_allocator_print(mem);
    QAK_CHECK(qak_allocator_num_allocated_bytes(mem) == 0, "Expected no allocated memory.");
}

void testModule() {
    printf("========= Test: parser simple module\n");
    qak_allocator mem = qak_heap_allocator_init();

    qak_source *source = qak_io_read_source_from_file(&mem, "data/parser_module.qak");
    QAK_CHECK(source, "Couldn't read test file data/parser_module.qak");

    qak_array_token *tokens = qak_array_token_new(&mem, 16);
    qak_errors errors = qak_errors_init(&mem);
    qak_allocator bump = qak_bump_allocator_init(sizeof(qak_ast_node) * 16);
    qak_ast_module *module = qak_parse(source, tokens, &errors, &bump);
    QAK_CHECK(module, "Expected module, got null pointer.");

    qak_allocator_shutdown(&bump);
    qak_allocator_shutdown(&mem);
    printf("SUCCESS\n");
}

void testFunction() {
    printf("========= Test: parse function\n");
    qak_allocator mem = qak_heap_allocator_init();

    {
        parser_test_data data = createTestData(&mem, "module test\nfunction foo()\nend");
        qak_ast_module *module = data.module;
        QAK_CHECK(module, "Expected module, got null pointer.");

        QAK_CHECK(module->numFunctions == 1, "Expected 1 function.");
        qak_ast_function *function = module->functions;
        QAK_CHECK(function->info.next == NULL, "Expected 1 function.");
        QAK_CHECK(qak_span_matches(&function->name, QAK_STR("foo")), "Expected function name 'foo'.");
        QAK_CHECK(function->parameters == NULL, "Expected 0 parameters.");
        QAK_CHECK(function->numParameters == 0, "Expected 0 parameters.");
        QAK_CHECK(function->returnType == NULL, "Expected no return type.");
        QAK_CHECK(function->statements == NULL, "Expected 0 statements.");
        QAK_CHECK(function->numStatements == 0, "Expected 0 statements.");

        destroyTestData(&mem, data);
    }

    {
        parser_test_data data = createTestData(&mem, "module test\nfunction foo(): int32\nend");
        qak_ast_module *module = data.module;
        QAK_CHECK(module, "Expected module, got null pointer.");

        QAK_CHECK(module->numFunctions == 1, "Expected 1 function.");
        qak_ast_function *function = module->functions;
        QAK_CHECK(function->info.next == NULL, "Expected 1 function.");
        QAK_CHECK(qak_span_matches(&function->name, QAK_STR("foo")), "Expected function name 'foo'.");
        QAK_CHECK(function->parameters == NULL, "Expected 0 parameters.");
        QAK_CHECK(function->numParameters == 0, "Expected 0 parameters.");
        QAK_CHECK(function->statements == NULL, "Expected 0 statements.");
        QAK_CHECK(function->numStatements == 0, "Expected 0 statements.");

        QAK_CHECK(function->returnType, "Expected return type.");
        QAK_CHECK(qak_span_matches(&function->returnType->name, QAK_STR("int32")),
                  "Expected int32 return type.");

        destroyTestData(&mem, data);
    }

    {
        parser_test_data data = createTestData(&mem, "module test\nfunction foo(a: int32): int32\nend");
        qak_ast_module *module = data.module;
        QAK_CHECK(module, "Expected module, got null pointer.");

        QAK_CHECK(module->numFunctions == 1, "Expected 1 function.");
        qak_ast_function *function = module->functions;
        QAK_CHECK(function->info.next == NULL, "Expected 1 function.");
        QAK_CHECK(qak_span_matches(&function->name, QAK_STR("foo")), "Expected function name 'foo'.");
        QAK_CHECK(function->statements == NULL, "Expected 0 statements.");
        QAK_CHECK(function->numStatements == 0, "Expected 0 statements.");

        QAK_CHECK(function->returnType, "Expected return type.");
        QAK_CHECK(qak_span_matches(&function->returnType->name, QAK_STR("int32")),
                  "Expected int32 return type.");

        QAK_CHECK(function->parameters, "Expected parameters.");
        QAK_CHECK(function->numParameters == 1, "Expected 1 parameter.");
        qak_ast_parameter *parameter = function->parameters;
        QAK_CHECK(parameter->info.next == NULL, "Expected 1 parameter.");
        QAK_CHECK(qak_span_matches(&parameter->name, QAK_STR("a")), "Expected parameter a.");
        QAK_CHECK(qak_span_matches(&parameter->typeSpecifier->name, QAK_STR("int32")),
                  "Expected parameter a.");

        destroyTestData(&mem, data);
    }

    {
        parser_test_data data = createTestData(&mem,
                                               "module test\nfunction foo(a: int32, b: float, c: double, d: int32): int32\nend");
        qak_ast_module *module = data.module;
        QAK_CHECK(module, "Expected module, got null pointer.");

        QAK_CHECK(module->numFunctions == 1, "Expected 1 function.");
        qak_ast_function *function = module->functions;
        QAK_CHECK(function->info.next == NULL, "Expected 1 function.");
        QAK_CHECK(qak_span_matches(&function->name, QAK_STR("foo")), "Expected function name 'foo'.");
        QAK_CHECK(function->statements == NULL, "Expected 0 statements.");
        QAK_CHECK(function->numStatements == 0, "Expected 0 statements.");

        QAK_CHECK(function->returnType, "Expected return type.");
        QAK_CHECK(qak_span_matches(&function->returnType->name, QAK_STR("int32")),
                  "Expected int32 return type.");

        QAK_CHECK(function->parameters, "Expected parameters.");
        QAK_CHECK(function->numParameters == 4, "Expected 4 parameter.");
        qak_ast_parameter *parameter = function->parameters;
        QAK_CHECK(parameter->info.next, "Expected more parameters.");
        QAK_CHECK(qak_span_matches(&parameter->name, QAK_STR("a")), "Expected parameter a.");
        QAK_CHECK(qak_span_matches(&parameter->typeSpecifier->name, QAK_STR("int32")),
                  "Expected type int32.");
        parameter = (qak_ast_parameter *) parameter->info.next;

        QAK_CHECK(parameter->info.next, "Expected more parameters.");
        QAK_CHECK(qak_span_matches(&parameter->name, QAK_STR("b")), "Expected parameter b.");
        QAK_CHECK(qak_span_matches(&parameter->typeSpecifier->name, QAK_STR("float")),
                  "Expected type float.");
        parameter = (qak_ast_parameter *) parameter->info.next;

        QAK_CHECK(parameter->info.next, "Expected more parameters.");
        QAK_CHECK(qak_span_matches(&parameter->name, QAK_STR("c")), "Expected parameter c.");
        QAK_CHECK(
                qak_span_matches(&parameter->typeSpecifier->name, QAK_STR("double")),
                "Expected type double.");
        parameter = (qak_ast_parameter *) parameter->info.next;

        QAK_CHECK(parameter->info.next == NULL, "Expected no more parameters.");
        QAK_CHECK(qak_span_matches(&parameter->name, QAK_STR("d")), "Expected parameter d.");
        QAK_CHECK(qak_span_matches(&parameter->typeSpecifier->name, QAK_STR("int32")),
                  "Expected type int32.");

        destroyTestData(&mem, data);
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
        parser_test_data data = createTestData(&mem, "module test\nvar a");
        qak_ast_module *module = data.module;
        QAK_CHECK(module, "Expected module, got null pointer.");

        QAK_CHECK(module->numStatements == 1, "Expected 1 statement.");
        qak_ast_variable *variable = (qak_ast_variable *) module->statements;
        QAK_CHECK(variable->info.type == QakAstVariable, "Expected variable AST node.");
        QAK_CHECK(qak_span_matches(&variable->name, QAK_STR("a")), "Expected variable a.");
        QAK_CHECK(variable->typeSpecifier == NULL, "Expected no type specifier.");
        QAK_CHECK(variable->initializerExpression == NULL, "Expected no initializer.");

        qak_module_print_ast(module);

        destroyTestData(&mem, data);
    }

    {
        parser_test_data data = createTestData(&mem, "module test\nvar a: int32");
        qak_ast_module *module = data.module;
        QAK_CHECK(module, "Expected module, got null pointer.");

        QAK_CHECK(module->numStatements == 1, "Expected 1 statement.");
        qak_ast_variable *variable = (qak_ast_variable *) module->statements;
        QAK_CHECK(variable->info.type == QakAstVariable, "Expected variable AST node.");
        QAK_CHECK(qak_span_matches(&variable->name, QAK_STR("a")), "Expected variable a.");
        QAK_CHECK(variable->typeSpecifier, "Expected type specifier.");
        QAK_CHECK(qak_span_matches(&variable->typeSpecifier->name, QAK_STR("int32")),
                  "Expected int32 type.");
        QAK_CHECK(variable->initializerExpression == NULL, "Expected no initializer.");

        destroyTestData(&mem, data);
    }

    {
        parser_test_data data = createTestData(&mem, "module test\nvar a = 0");
        qak_ast_module *module = data.module;
        QAK_CHECK(module, "Expected module, got null pointer.");

        QAK_CHECK(module->numStatements == 1, "Expected 1 statement.");
        qak_ast_variable *variable = (qak_ast_variable *) module->statements;
        QAK_CHECK(variable->info.type == QakAstVariable, "Expected variable AST node.");
        QAK_CHECK(qak_span_matches(&variable->name, QAK_STR("a")), "Expected variable a.");
        QAK_CHECK(!variable->typeSpecifier, "Did not expect type specifier.");
        QAK_CHECK(variable->initializerExpression, "Expected initializer.");
        QAK_CHECK(variable->initializerExpression->type == QakAstLiteral,
                  "Expected literal initializer.");
        qak_ast_literal *literal = (qak_ast_literal *) variable->initializerExpression;
        QAK_CHECK(literal->type == QakTokenIntegerLiteral,
                  "Expected int literal.");
        QAK_CHECK(literal->value.data.data[0] == '0',
                  "Expected 0 literal.");
        QAK_CHECK(literal->value.data.length == 1,
                  "Expected 1-character literal.");

        qak_module_print_ast(module);

        destroyTestData(&mem, data);
    }

    {
        parser_test_data data = createTestData(&mem, "module test\nvar a: int32 = 0");
        qak_ast_module *module = data.module;
        QAK_CHECK(module, "Expected module, got null pointer.");

        QAK_CHECK(module->numStatements == 1, "Expected 1 statement.");
        qak_ast_variable *variable = (qak_ast_variable *) module->statements;
        QAK_CHECK(variable->info.type == QakAstVariable, "Expected variable AST node.");
        QAK_CHECK(qak_span_matches(&variable->name, QAK_STR("a")), "Expected variable a.");
        QAK_CHECK(variable->typeSpecifier, "Expected type specifier.");
        QAK_CHECK(qak_span_matches(&variable->typeSpecifier->name, QAK_STR("int32")),
                  "Expected int32 type.");
        QAK_CHECK(variable->initializerExpression, "Expected initializer.");
        QAK_CHECK(variable->initializerExpression->type == QakAstLiteral,
                  "Expected literal initializer.");
        qak_ast_literal *literal = (qak_ast_literal *) variable->initializerExpression;
        QAK_CHECK(literal->type == QakTokenIntegerLiteral,
                  "Expected int literal.");
        QAK_CHECK(literal->value.data.data[0] == '0',
                  "Expected 0 literal.");
        QAK_CHECK(literal->value.data.length == 1,
                  "Expected 1-character literal.");

        qak_module_print_ast(module);

        destroyTestData(&mem, data);
    }

    qak_allocator_print(&mem);
    QAK_CHECK(qak_allocator_num_allocated_bytes(&mem) == 0, "Parser shutdown didn't deallocate all memory.");

    qak_allocator_shutdown(&mem);
    printf("SUCCESS\n");
}

void testExpression() {
    printf("========= Test: parser expression\n");
    qak_allocator mem = qak_heap_allocator_init();

    {
        parser_test_data data = createTestData(&mem, "module test\n0");
        qak_ast_module *module = data.module;
        QAK_CHECK(module, "Expected module, got null pointer.");

        QAK_CHECK(module->numStatements == 1, "Expected 1 statement.");
        qak_ast_literal *literal = (qak_ast_literal *) module->statements;
        QAK_CHECK(literal, "Expected literal.");
        QAK_CHECK(literal->info.type == QakAstLiteral, "Expected literal.");
        QAK_CHECK(literal->type == QakTokenIntegerLiteral, "Expected integer literal.");
        QAK_CHECK(literal->value.data.data[0] == '0', "Expected 0 literal.");

        destroyTestData(&mem, data);
    }

    {
        parser_test_data data = createTestData(&mem, "module test\n0b");
        qak_ast_module *module = data.module;
        QAK_CHECK(module, "Expected module, got null pointer.");

        QAK_CHECK(module->numStatements == 1, "Expected 1 statement.");
        qak_ast_literal *literal = (qak_ast_literal *) module->statements;
        QAK_CHECK(literal, "Expected literal.");
        QAK_CHECK(literal->info.type == QakAstLiteral, "Expected literal.");
        QAK_CHECK(literal->type == QakTokenByteLiteral, "Expected byte literal.");
        QAK_CHECK(literal->value.data.data[0] == '0', "Expected 0b literal.");
        QAK_CHECK(literal->value.data.data[1] == 'b', "Expected 0b literal.");

        destroyTestData(&mem, data);
    }

    {
        parser_test_data data = createTestData(&mem, "module test\n0s");
        qak_ast_module *module = data.module;
        QAK_CHECK(module, "Expected module, got null pointer.");

        QAK_CHECK(module->numStatements == 1, "Expected 1 statement.");
        qak_ast_literal *literal = (qak_ast_literal *) module->statements;
        QAK_CHECK(literal, "Expected literal.");
        QAK_CHECK(literal->info.type == QakAstLiteral, "Expected literal.");
        QAK_CHECK(literal->type == QakTokenShortLiteral, "Expected short literal.");
        QAK_CHECK(literal->value.data.data[0] == '0', "Expected 0s literal.");
        QAK_CHECK(literal->value.data.data[1] == 's', "Expected 0s literal.");

        destroyTestData(&mem, data);
    }

    {
        parser_test_data data = createTestData(&mem, "module test\n0l");
        qak_ast_module *module = data.module;
        QAK_CHECK(module, "Expected module, got null pointer.");

        QAK_CHECK(module->numStatements == 1, "Expected 1 statement.");
        qak_ast_literal *literal = (qak_ast_literal *) module->statements;
        QAK_CHECK(literal, "Expected literal.");
        QAK_CHECK(literal->info.type == QakAstLiteral, "Expected literal.");
        QAK_CHECK(literal->type == QakTokenLongLiteral, "Expected long literal.");
        QAK_CHECK(literal->value.data.data[0] == '0', "Expected 0l literal.");
        QAK_CHECK(literal->value.data.data[1] == 'l', "Expected 0l literal.");

        destroyTestData(&mem, data);
    }

    {
        parser_test_data data = createTestData(&mem, "module test\n0.0");
        qak_ast_module *module = data.module;
        QAK_CHECK(module, "Expected module, got null pointer.");

        QAK_CHECK(module->numStatements == 1, "Expected 1 statement.");
        qak_ast_literal *literal = (qak_ast_literal *) module->statements;
        QAK_CHECK(literal, "Expected literal.");
        QAK_CHECK(literal->info.type == QakAstLiteral, "Expected literal.");
        QAK_CHECK(literal->type == QakTokenFloatLiteral, "Expected float literal.");
        QAK_CHECK(strncmp(literal->value.data.data, "0.0", 3) == 0, "Expected 0.0 literal.");

        destroyTestData(&mem, data);
    }

    {
        parser_test_data data = createTestData(&mem, "module test\n0.0f");
        qak_ast_module *module = data.module;
        QAK_CHECK(module, "Expected module, got null pointer.");

        QAK_CHECK(module->numStatements == 1, "Expected 1 statement.");
        qak_ast_literal *literal = (qak_ast_literal *) module->statements;
        QAK_CHECK(literal, "Expected literal.");
        QAK_CHECK(literal->info.type == QakAstLiteral, "Expected literal.");
        QAK_CHECK(literal->type == QakTokenFloatLiteral, "Expected float literal.");
        QAK_CHECK(strncmp(literal->value.data.data, "0.0f", 4) == 0, "Expected 0.0f literal.");

        destroyTestData(&mem, data);
    }

    {
        parser_test_data data = createTestData(&mem, "module test\n0.0d");
        qak_ast_module *module = data.module;
        QAK_CHECK(module, "Expected module, got null pointer.");

        QAK_CHECK(module->numStatements == 1, "Expected 1 statement.");
        qak_ast_literal *literal = (qak_ast_literal *) module->statements;
        QAK_CHECK(literal, "Expected literal.");
        QAK_CHECK(literal->info.type == QakAstLiteral, "Expected literal.");
        QAK_CHECK(literal->type == QakTokenDoubleLiteral, "Expected float literal.");
        QAK_CHECK(strncmp(literal->value.data.data, "0.0d", 4) == 0, "Expected 0.0d literal.");

        destroyTestData(&mem, data);
    }

    {
        parser_test_data data = createTestData(&mem, "module test\n'c'");
        qak_ast_module *module = data.module;
        QAK_CHECK(module, "Expected module, got null pointer.");

        QAK_CHECK(module->numStatements == 1, "Expected 1 statement.");
        qak_ast_literal *literal = (qak_ast_literal *) module->statements;
        QAK_CHECK(literal, "Expected literal.");
        QAK_CHECK(literal->info.type == QakAstLiteral, "Expected literal.");
        QAK_CHECK(literal->type == QakTokenCharacterLiteral, "Expected char literal.");
        QAK_CHECK(strncmp(literal->value.data.data, "'c'", 3) == 0, "Expected 'c' literal.");

        destroyTestData(&mem, data);
    }

    {
        parser_test_data data = createTestData(&mem, "module test\n\"Hello\"");
        qak_ast_module *module = data.module;
        QAK_CHECK(module, "Expected module, got null pointer.");

        QAK_CHECK(module->numStatements == 1, "Expected 1 statement.");
        qak_ast_literal *literal = (qak_ast_literal *) module->statements;
        QAK_CHECK(literal, "Expected literal.");
        QAK_CHECK(literal->info.type == QakAstLiteral, "Expected literal.");
        QAK_CHECK(literal->type == QakTokenStringLiteral, "Expected string literal.");
        QAK_CHECK(strncmp(literal->value.data.data, "\"Hello\"", 7) == 0, "Expected \"Hello\" literal.");

        destroyTestData(&mem, data);
    }

    {
        parser_test_data data = createTestData(&mem, "module test\ntrue");
        qak_ast_module *module = data.module;
        QAK_CHECK(module, "Expected module, got null pointer.");

        QAK_CHECK(module->numStatements == 1, "Expected 1 statement.");
        qak_ast_literal *literal = (qak_ast_literal *) module->statements;
        QAK_CHECK(literal, "Expected literal.");
        QAK_CHECK(literal->info.type == QakAstLiteral, "Expected literal.");
        QAK_CHECK(literal->type == QakTokenBooleanLiteral, "Expected boolean literal.");
        QAK_CHECK(strncmp(literal->value.data.data, "true", 4) == 0, "Expected true literal.");

        destroyTestData(&mem, data);
    }

    {
        parser_test_data data = createTestData(&mem, "module test\nnothing");
        qak_ast_module *module = data.module;
        QAK_CHECK(module, "Expected module, got null pointer.");

        QAK_CHECK(module->numStatements == 1, "Expected 1 statement.");
        qak_ast_literal *literal = (qak_ast_literal *) module->statements;
        QAK_CHECK(literal, "Expected literal.");
        QAK_CHECK(literal->info.type == QakAstLiteral, "Expected literal.");
        QAK_CHECK(literal->type == QakTokenNothingLiteral, "Expected nothing literal.");
        QAK_CHECK(strncmp(literal->value.data.data, "nothing", 7) == 0, "Expected nothing literal.");

        destroyTestData(&mem, data);
    }

    {
        parser_test_data data = createTestData(&mem, "module test\n-1");
        qak_ast_module *module = data.module;
        QAK_CHECK(module, "Expected module, got null pointer.");

        QAK_CHECK(module->numStatements == 1, "Expected 1 statement.");
        qak_ast_unary_operation *op = (qak_ast_unary_operation *) module->statements;
        QAK_CHECK(op->info.type = QakAstUnaryOperation, "Expected unary operation.");
        QAK_CHECK(op->opType == QakTokenMinus, "Expected - unary operation.");
        QAK_CHECK(op->value->type == QakAstLiteral, "Expected literal.");

        destroyTestData(&mem, data);
    }

    {
        parser_test_data data = createTestData(&mem, "module test\n!1");
        qak_ast_module *module = data.module;
        QAK_CHECK(module, "Expected module, got null pointer.");

        QAK_CHECK(module->numStatements == 1, "Expected 1 statement.");
        qak_ast_unary_operation *op = (qak_ast_unary_operation *) module->statements;
        QAK_CHECK(op->info.type = QakAstUnaryOperation, "Expected unary operation.");
        QAK_CHECK(op->opType == QakTokenNot, "Expected ! unary operation.");
        QAK_CHECK(op->value->type == QakAstLiteral, "Expected literal.");

        destroyTestData(&mem, data);
    }

    {
        parser_test_data data = createTestData(&mem, "module test\n!-1");
        qak_ast_module *module = data.module;
        QAK_CHECK(module, "Expected module, got null pointer.");

        QAK_CHECK(module->numStatements == 1, "Expected 1 statement.");
        qak_ast_unary_operation *op = (qak_ast_unary_operation *) module->statements;
        QAK_CHECK(op->info.type = QakAstUnaryOperation, "Expected unary operation.");
        QAK_CHECK(op->opType == QakTokenNot, "Expected ! unary operation.");
        QAK_CHECK(op->value->type == QakAstUnaryOperation, "Expected - unary operation.");
        QAK_CHECK(((qak_ast_unary_operation *) op->value)->opType == QakTokenMinus,
                  "Expected - unary operation.");

        destroyTestData(&mem, data);
    }

    QAK_CHECK(qak_allocator_num_allocated_bytes(&mem) == 0, "Expected no allocated memory.");
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
    testExpression();
    return 0;
}
