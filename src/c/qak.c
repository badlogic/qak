#include "qak.h"
#include "io.h"
#include "error.h"
#include "tokenizer.h"
#include "parser.h"
#include <stdio.h>

#define QAK_AST_INDENT 2

qak_module *compile(qak_allocator mem, qak_source *source) {
    qak_module *module = QAK_ALLOCATE(&mem, qak_module, 1);
    module->source = source;
    module->mem = mem; // transfer ownership of allocator to module
    module->bumpMem = qak_bump_allocator_init(sizeof(qak_ast_node) * 256);
    module->errors = qak_errors_init(&module->mem);
    module->tokens = qak_array_token_new(&module->mem, 16);
    module->ast = NULL;

    qak_tokenize(source, module->tokens, &module->errors);
    if (module->errors.errors->size) return module;

    module->ast = qak_parse(source, module->tokens, &module->errors, &module->bumpMem);
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

static void printIndent(uint32_t indent) {
    printf("%*s", indent, "");
}

static void print_ast_recursive(qak_ast_node *node, uint32_t indent) {
    switch (node->type) {
        case QakAstTypeSpecifier: {
            qak_ast_type_specifier *n = &node->data.typeSpecifier;
            printIndent(indent);
            printf("type: %.*s\n", (int) n->name.data.length, n->name.data.data);
            break;
        }
        case QakAstParameter: {
            qak_ast_parameter *n = &node->data.parameter;
            printIndent(indent);
            printf("Parameter: %.*s\n", (int) n->name.data.length, n->name.data.data);
            print_ast_recursive(n->typeSpecifier, indent + QAK_AST_INDENT);
            break;
        }
        case QakAstFunction: {
            qak_ast_function *n = &node->data.function;
            printIndent(indent);
            printf("Function: %.*s\n", (int) n->name.data.length, n->name.data.data);
            if (n->numParameters > 0) {
                printIndent(indent + QAK_AST_INDENT);
                printf("Parameters:\n");
                qak_ast_node *parameter = n->parameters;
                while (parameter) {
                    print_ast_recursive(parameter, indent + QAK_AST_INDENT * 2);
                    parameter = parameter->next;
                }
            }
            if (n->returnType) {
                printIndent(indent + QAK_AST_INDENT);
                printf("Return type:\n");
                print_ast_recursive(n->returnType, indent + QAK_AST_INDENT * 2);
            }

            if (n->numStatements > 0) {
                printIndent(indent + QAK_AST_INDENT);
                printf("Statements:\n");
                qak_ast_node *statement = n->statements;
                while (statement) {
                    print_ast_recursive(statement, indent + QAK_AST_INDENT * 2);
                    statement = statement->next;
                }
            }
            break;
        }
        case QakAstVariable: {
            qak_ast_variable *n = &node->data.variable;
            printIndent(indent);
            printf("Variable: %.*s\n", (int) n->name.data.length, n->name.data.data);
            if (n->typeSpecifier) print_ast_recursive(n->typeSpecifier, indent + QAK_AST_INDENT);
            if (n->initializerExpression) {
                printIndent(indent + QAK_AST_INDENT);
                printf("Initializer: \n");
                print_ast_recursive(n->initializerExpression, indent + QAK_AST_INDENT * 2);
            }
            break;
        }
        case QakAstWhile: {
            qak_ast_while *n = &node->data.whileNode;
            printIndent(indent);
            printf("While\n");

            printIndent(indent + QAK_AST_INDENT);
            printf("Condition: \n");
            print_ast_recursive(n->condition, indent + QAK_AST_INDENT * 2);

            if (n->numStatements > 0) {
                printIndent(indent + QAK_AST_INDENT);
                printf("Statements:\n");
                qak_ast_node *statement = n->statements;
                while (statement) {
                    print_ast_recursive(statement, indent + QAK_AST_INDENT * 2);
                    statement = statement->next;
                }
            }
            break;
        }
        case QakAstIf: {
            qak_ast_if *n = &node->data.ifNode;
            printIndent(indent);
            printf("If\n");

            printIndent(indent + QAK_AST_INDENT);
            printf("Condition: \n");
            print_ast_recursive(n->condition, indent + QAK_AST_INDENT * 2);

            if (n->numTrueBlockStatements > 0) {
                printIndent(indent + QAK_AST_INDENT);
                printf("True-block statements: \n");
                qak_ast_node *statement = n->trueBlock;
                while (statement) {
                    print_ast_recursive(statement, indent + QAK_AST_INDENT * 2);
                    statement = statement->next;
                }
            }

            if (n->numFalseBlockStatements > 0) {
                printIndent(indent + QAK_AST_INDENT);
                printf("False-block statements: \n");
                qak_ast_node *statement = n->falseBlock;
                while (statement) {
                    print_ast_recursive(statement, indent + QAK_AST_INDENT * 2);
                    statement = statement->next;
                }
            }
            break;
        }
        case QakAstReturn: {
            qak_ast_return *n = &node->data.returnNode;
            printIndent(indent);
            printf("Return:\n");

            if (n->returnValue) {
                printIndent(indent + QAK_AST_INDENT);
                printf("Value:\n");
                print_ast_recursive(n->returnValue, indent + QAK_AST_INDENT * 2);
            }

            break;
        }
        case QakAstTernaryOperation: {
            qak_ast_ternary_operation *n = &node->data.ternaryOperation;
            printIndent(indent);
            printf("Ternary operator:\n");
            print_ast_recursive(n->condition, indent + QAK_AST_INDENT);
            print_ast_recursive(n->trueValue, indent + QAK_AST_INDENT);
            print_ast_recursive(n->falseValue, indent + QAK_AST_INDENT);
            break;
        }
        case QakAstBinaryOperation: {
            qak_ast_binary_operation *n = &node->data.binaryOperation;
            printIndent(indent);
            printf("Binary operator: %s\n", qak_token_type_to_string(n->opType));
            print_ast_recursive(n->left, indent + QAK_AST_INDENT);
            print_ast_recursive(n->right, indent + QAK_AST_INDENT);
            break;
        }
        case QakAstUnaryOperation: {
            qak_ast_unary_operation *n = &node->data.unaryOperation;
            printIndent(indent);
            printf("Unary op: %s\n", qak_token_type_to_string(n->opType));
            print_ast_recursive(n->value, indent + QAK_AST_INDENT);
            break;
        }
        case QakAstLiteral: {
            qak_ast_literal *n = &node->data.literal;
            printIndent(indent);
            printf("%s: %.*s\n", qak_token_type_to_string(n->type), (int) n->value.data.length, n->value.data.data);
            break;
        }
        case QakAstVariableAccess: {
            qak_ast_variable_access *n = &node->data.variableAccess;
            printIndent(indent);
            printf("Variable access: %.*s\n", (int) n->name.data.length, n->name.data.data);
            break;
        }
        case QakAstFunctionCall: {
            qak_ast_function_call *n = &node->data.functionCall;
            printIndent(indent);
            printf("Function call: %.*s(%s)\n", (int) n->variableAccess->span.data.length,
                   n->variableAccess->span.data.data, n->numArguments > 0 ? "..." : "");

            if (n->numArguments > 0) {
                printIndent(indent + QAK_AST_INDENT);
                printf("Arguments:\n");
                qak_ast_node *argument = n->arguments;
                while (argument) {
                    print_ast_recursive(argument, indent + QAK_AST_INDENT * 2);
                    argument = argument->next;
                }
            }
            break;
        }
        case QakAstModule: {
            qak_ast_module *n = &node->data.module;
            printIndent(indent);
            printf("Module: %.*s\n", (int) n->name.data.length, n->name.data.data);

            if (n->numStatements > 0) {
                printIndent(indent + QAK_AST_INDENT);
                printf("Module statements:\n");
                qak_ast_node *statement = n->statements;
                while (statement) {
                    print_ast_recursive(statement, indent + QAK_AST_INDENT * 2);
                    statement = statement->next;
                }
            }

            qak_ast_node *function = n->functions;
            while (function) {
                print_ast_recursive(function, indent + QAK_AST_INDENT);
                function = function->next;
            }
            break;
        }
    }
}

void qak_module_print_ast(qak_ast_node *moduleAst) {
    if (!moduleAst) return;

    qak_allocator mem = qak_heap_allocator_init();
    print_ast_recursive(moduleAst, 0);
    qak_allocator_shutdown(&mem);
}
