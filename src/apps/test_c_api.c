#include <stdio.h>
#include "c/qak.h"
#include "test.h"

#define INDENT 3

static void printIndent(int indent) {
    printf("%*s", indent, "");
}

static void printSpan(const char *label, qak_span *span) {
    printf("%s%.*s\n", label, (int) span->data.length, span->data.data);
}

static void printAstNodeRecursive(qak_module module, qak_ast_node *node, int indent) {
    switch (node->type) {
        case QakAstTypeSpecifier: {
            printIndent(indent);
            printSpan("Type: ", &node->data.typeSpecifier.name);
            break;
        }
        case QakAstParameter: {
            printIndent(indent);
            printSpan("Parameter: ", &node->data.parameter.name);
            printAstNodeRecursive(module, qak_module_get_ast_node(module, node->data.parameter.typeSpecifier), indent + INDENT);
            break;
        }
        case QakAstFunction: {
            printIndent(indent);
            printSpan("Function: ", &node->data.function.name);
            if (node->data.function.parameters.numNodes > 0) {
                printIndent(indent + INDENT);
                printf("Parameters:\n");
                for (size_t i = 0; i < node->data.function.parameters.numNodes; i++)
                    printAstNodeRecursive(module, qak_module_get_ast_node(module, node->data.function.parameters.nodes[i]), indent + INDENT * 2);
            }
            if (node->data.function.returnType >= 0) {
                printIndent(indent + INDENT);
                printf("Return type:\n");
                printAstNodeRecursive(module, qak_module_get_ast_node(module, node->data.function.returnType), indent + INDENT * 2);
            }

            if (node->data.function.statements.numNodes > 0) {
                printIndent(indent + INDENT);
                printf("Statements:\n");
                for (size_t i = 0; i < node->data.function.statements.numNodes; i++) {
                    printAstNodeRecursive(module, qak_module_get_ast_node(module, node->data.function.statements.nodes[i]), indent + INDENT * 2);
                }
            }
            break;
        }
        case QakAstVariable: {
            printIndent(indent);
            printSpan("Variable: ", &node->data.variable.name);
            if (node->data.variable.typeSpecifier >= 0)
                printAstNodeRecursive(module, qak_module_get_ast_node(module, node->data.variable.typeSpecifier), indent + INDENT);
            if (node->data.variable.initializerExpression >= 0) {
                printIndent(indent + INDENT);
                printf("Initializer: \n");
                printAstNodeRecursive(module, qak_module_get_ast_node(module, node->data.variable.initializerExpression), indent + INDENT * 2);
            }
            break;
        }
        case QakAstWhile: {
            printIndent(indent);
            printf("While\n");

            printIndent(indent + INDENT);
            printf("Condition: \n");
            printAstNodeRecursive(module, qak_module_get_ast_node(module, node->data.whileNode.condition), indent + INDENT * 2);

            if (node->data.whileNode.statements.numNodes > 0) {
                printIndent(indent + INDENT);
                printf("Statements: \n");
                for (size_t i = 0; i < node->data.whileNode.statements.numNodes; i++) {
                    printAstNodeRecursive(module, qak_module_get_ast_node(module, node->data.whileNode.statements.nodes[i]), indent + INDENT * 2);
                }
            }
            break;
        }
        case QakAstIf: {
            printIndent(indent);
            printf("If\n");

            printIndent(indent + INDENT);
            printf("Condition: \n");
            printAstNodeRecursive(module, qak_module_get_ast_node(module, node->data.ifNode.condition), indent + INDENT * 2);

            if (node->data.ifNode.trueBlock.numNodes > 0) {
                printIndent(indent + INDENT);
                printf("True-block statements: \n");
                for (size_t i = 0; i < node->data.ifNode.trueBlock.numNodes; i++) {
                    printAstNodeRecursive(module, qak_module_get_ast_node(module, node->data.ifNode.trueBlock.nodes[i]), indent + INDENT * 2);
                }
            }

            if (node->data.ifNode.falseBlock.numNodes > 0) {
                printIndent(indent + INDENT);
                printf("False-block statements: \n");
                for (size_t i = 0; i < node->data.ifNode.falseBlock.numNodes; i++) {
                    printAstNodeRecursive(module, qak_module_get_ast_node(module, node->data.ifNode.falseBlock.nodes[i]), indent + INDENT * 2);
                }
            }
            break;
        }
        case QakAstReturn: {
            printIndent(indent);
            printf("Return:\n");

            if (node->data.returnNode.returnValue) {
                printIndent(indent + INDENT);
                printf("Value:\n");
                printAstNodeRecursive(module, qak_module_get_ast_node(module, node->data.returnNode.returnValue), indent + INDENT * 2);
            }

            break;
        }
        case QakAstTernaryOperation: {
            printIndent(indent);
            printf("Ternary operator:\n");
            printAstNodeRecursive(module, qak_module_get_ast_node(module, node->data.ternaryOperation.condition), indent + INDENT);
            printAstNodeRecursive(module, qak_module_get_ast_node(module, node->data.ternaryOperation.trueValue), indent + INDENT);
            printAstNodeRecursive(module, qak_module_get_ast_node(module, node->data.ternaryOperation.falseValue), indent + INDENT);
            break;
        }
        case QakAstBinaryOperation: {
            printIndent(indent);
            printSpan("Binary operator: ", &node->data.binaryOperation.op);
            printAstNodeRecursive(module, qak_module_get_ast_node(module, node->data.binaryOperation.left), indent + INDENT);
            printAstNodeRecursive(module, qak_module_get_ast_node(module, node->data.binaryOperation.right), indent + INDENT);
            break;
        }
        case QakAstUnaryOperation: {
            printIndent(indent);
            printSpan("Unary op: ", &node->data.unaryOperation.op);
            printAstNodeRecursive(module, qak_module_get_ast_node(module, node->data.unaryOperation.value), indent + INDENT);
            break;
        }
        case QakAstLiteral: {
            printIndent(indent);
            printSpan("", &node->data.literal.value);
            break;
        }
        case QakAstVariableAccess: {
            printIndent(indent);
            printSpan("Variable access: ", &node->data.variableAccess.name);
            break;
        }
        case QakAstFunctionCall: {
            printIndent(indent);
            printSpan("Function call: ", &node->span);

            if (node->data.functionCall.arguments.numNodes > 0) {
                printIndent(indent + INDENT);
                printf("Arguments:\n");
                for (size_t i = 0; i < node->data.functionCall.arguments.numNodes; i++) {
                    printAstNodeRecursive(module, qak_module_get_ast_node(module, node->data.functionCall.arguments.nodes[i]), indent + INDENT * 2);
                }
            }
            break;
        }
        case QakAstModule: {
            printIndent(indent);
            printSpan("Module: ", &node->data.module.name);

            if (node->data.module.statements.numNodes > 0) {
                printIndent(indent + INDENT);
                printf("Module statements:\n");
                for (size_t i = 0; i < node->data.module.statements.numNodes; i++) {
                    printAstNodeRecursive(module, qak_module_get_ast_node(module, node->data.module.statements.nodes[i]), indent + INDENT * 2);
                }
            }

            for (size_t i = 0; i < node->data.module.functions.numNodes; i++) {
                printAstNodeRecursive(module, qak_module_get_ast_node(module, node->data.module.functions.nodes[i]), indent + INDENT);
            }
            break;
        }
    }
}

int main() {
    qak_compiler compiler = qak_compiler_new();
    QAK_CHECK(compiler, "Couldn't create compiler");

    qak_module module = qak_compiler_compile_file(compiler, "data/parser_function.qak");
    QAK_CHECK(module, "Couldn't parse module");

    qak_ast_module *astModule = qak_module_get_ast(module);
    QAK_CHECK(astModule, "Couldn't get module AST");

    printSpan("Module: ", &astModule->name);

    printf("Functions: %i\n", astModule->functions.numNodes);
    for (size_t i = 0; i < astModule->functions.numNodes; i++) {
        printAstNodeRecursive(module, qak_module_get_ast_node(module, astModule->functions.nodes[i]), 1);
    }

    printf("Variables: %i\n", astModule->variables.numNodes);
    for (size_t i = 0; i < astModule->variables.numNodes; i++) {
        printAstNodeRecursive(module, qak_module_get_ast_node(module, astModule->variables.nodes[i]), 1);
    }

    printf("Statements: %i\n", astModule->statements.numNodes);
    for (size_t i = 0; i < astModule->statements.numNodes; i++) {
        printAstNodeRecursive(module, qak_module_get_ast_node(module, astModule->statements.nodes[i]), 1);
    }

    qak_module_delete(module);

    qak_compiler_delete(compiler);
}
