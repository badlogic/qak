#pragma once

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void *qak_compiler;

typedef void *qak_module;

/** A UTF-8 string from a Source with length bytes. **/
typedef struct qak_string {
    char *data;
    size_t length;
} qak_string;

typedef struct qak_span {
    qak_string data;
    uint32_t startLine;
    uint32_t endLine;
} qak_span;

typedef struct qak_line {
    qak_string data;
    uint32_t lineNumber;
} qak_line;

QAK_ARRAY_IMPLEMENT_INLINE(qak_array_line, qak_line)

/** A source file with a name **/
typedef struct qak_source {
    qak_allocator *allocator;
    qak_string data;
    qak_string fileName;
    qak_array_line *lines;
} qak_source;

typedef enum qak_token_type {
    QakTokenPeriod = 0,
    QakTokenComma,
    QakTokenSemicolon,
    QakTokenColon,
    QakTokenPlus,
    QakTokenMinus,
    QakTokenAsterisk,
    QakTokenForwardSlash,
    QakTokenPercentage,
    QakTokenLeftParenthesis,
    QakTokenRightParenthesis,
    QakTokenLeftBracket,
    QakTokenRightBracket,
    QakTokenLeftCurly,
    QakTokenRightCurly,
    QakTokenLessEqual,
    QakTokenGreaterEqual,
    QakTokenNotEqual,
    QakTokenEqual,
    QakTokenLess,
    QakTokenGreater,
    QakTokenAssignment,
    QakTokenAnd,
    QakTokenOr,
    QakTokenXor,
    QakTokenNot,
    QakTokenQuestionMark,
    QakTokenUnknown,

    QakTokenBooleanLiteral,
    QakTokenDoubleLiteral,
    QakTokenFloatLiteral,
    QakTokenLongLiteral,
    QakTokenIntegerLiteral,
    QakTokenShortLiteral,
    QakTokenByteLiteral,
    QakTokenCharacterLiteral,
    QakTokenStringLiteral,
    QakTokenNothingLiteral,
    QakTokenIdentifier
} qak_token_type;

typedef struct qak_token {
    qak_token_type type;
    qak_span span;
} qak_token;

QAK_ARRAY_IMPLEMENT_INLINE(qak_array_token, qak_token)

typedef enum qak_ast_type {
    QakAstTypeSpecifier,
    QakAstParameter,
    QakAstFunction,
    QakAstTernaryOperation,
    QakAstBinaryOperation,
    QakAstUnaryOperation,
    QakAstLiteral,
    QakAstVariableAccess,
    QakAstFunctionCall,
    QakAstVariable,
    QakAstWhile,
    QakAstIf,
    QakAstReturn,
    QakAstModule
} qak_ast_type;

typedef int32_t qak_ast_node_index;

typedef struct qak_ast_node_list {
    uint32_t numNodes;
    qak_ast_node_index *nodes;
} qak_ast_node_list;

typedef struct qak_ast_type_specifier {
    qak_span name;
} qak_ast_type_specifier;

typedef struct qak_ast_parameter {
    qak_span name;
    qak_ast_node_index typeSpecifier;
} qak_ast_parameter;

typedef struct qak_ast_function {
    qak_span name;
    qak_ast_node_list parameters;
    qak_ast_node_index returnType;
    qak_ast_node_list statements;
} qak_ast_function;

typedef struct qak_ast_variable {
    qak_span name;
    qak_ast_node_index typeSpecifier;
    qak_ast_node_index initializerExpression;
} qak_ast_variable;

typedef struct qak_ast_while {
    qak_ast_node_index condition;
    qak_ast_node_list statements;
} qak_ast_while;

typedef struct qak_ast_if {
    qak_ast_node_index condition;
    qak_ast_node_list trueBlock;
    qak_ast_node_list falseBlock;
} qak_ast_if;

typedef struct qak_ast_return {
    qak_ast_node_index returnValue;
} qak_ast_return;

typedef struct qak_ast_ternary_operation {
    qak_ast_node_index condition;
    qak_ast_node_index trueValue;
    qak_ast_node_index falseValue;
} qak_ast_ternary_operation;

typedef struct qak_ast_binary_operation {
    qak_span op;
    qak_ast_node_index left;
    qak_ast_node_index right;
} qak_ast_binary_operation;

typedef struct qak_ast_unary_operation {
    qak_span op;
    qak_ast_node_index value;
} qak_ast_unary_operation;

typedef struct qak_ast_literal {
    qak_token_type type;
    qak_span value;
} qak_ast_literal;

typedef struct qak_ast_variable_access {
    qak_span name;
} qak_ast_variable_access;

typedef struct qak_ast_function_call {
    qak_ast_node_index variableAccess;
    qak_ast_node_list arguments;
} qak_ast_function_call;

typedef struct qak_ast_module {
    qak_span name;
    qak_ast_node_list variables;
    qak_ast_node_list functions;
    qak_ast_node_list statements;
} qak_ast_module;

typedef struct qak_ast_node {
    qak_ast_type type;
    qak_span span;
    union {
        qak_ast_type_specifier typeSpecifier;
        qak_ast_parameter parameter;
        qak_ast_function function;
        qak_ast_variable variable;
        qak_ast_while whileNode;
        qak_ast_if ifNode;
        qak_ast_return returnNode;
        qak_ast_ternary_operation ternaryOperation;
        qak_ast_binary_operation binaryOperation;
        qak_ast_unary_operation unaryOperation;
        qak_ast_literal literal;
        qak_ast_variable_access variableAccess;
        qak_ast_function_call functionCall;
        qak_ast_module module;
    } data;
} qak_ast_node;

QAK_ARRAY_IMPLEMENT_INLINE(qak_array_ast_node, qak_ast_node)

typedef struct qak_error {
    qak_source *source;
    qak_string errorMessage;
    qak_span span;
} qak_error;

QAK_ARRAY_IMPLEMENT_INLINE(qak_array_error, qak_error)

typedef struct qak_errors {
    qak_allocator *allocator;
    qak_array_error *errors;
} qak_errors;

/** Compiler **/
qak_compiler qak_compiler_new();

void qak_compiler_delete(qak_compiler compiler);

void qak_compiler_print_memory_usage(qak_compiler compile);

qak_module qak_compiler_compile_file(qak_compiler compiler, const char *fileName);

qak_module qak_compiler_compile_source(qak_compiler compiler, const char *fileName, const char *source);

/** Module **/
void qak_module_delete(qak_module module);

void qak_module_get_source(qak_module module, qak_source *source);

int qak_module_get_num_errors(qak_module module);

void qak_module_get_error(qak_module moduleHandle, int errorIndex, qak_error *error);

void qak_module_print_errors(qak_module module);

int qak_module_get_num_tokens(qak_module module);

void qak_module_get_token(qak_module moduleHandle, int tokenIndex, qak_token *token);

void qak_module_print_tokens(qak_module module);

qak_ast_module *qak_module_get_ast(qak_module module);

qak_ast_node *qak_module_get_ast_node(qak_module module, qak_ast_node_index nodeIndex);

void qak_module_print_ast(qak_module module);

#ifdef WASM
void qak_print_struct_offsets();
#endif

#ifdef __cplusplus
}
#endif
