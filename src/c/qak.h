#pragma once

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

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

/** A source file with a name **/
typedef struct qak_source {
    qak_allocator *allocator;
    qak_string data;
    qak_string fileName;
    qak_line *lines;
    size_t numLines;
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

struct qak_ast_node;

typedef struct qak_ast_type_specifier {
    qak_span name;
} qak_ast_type_specifier;

typedef struct qak_ast_parameter {
    qak_span name;
    struct qak_ast_node *typeSpecifier;
} qak_ast_parameter;

typedef struct qak_ast_function {
    qak_span name;
    struct qak_ast_node *parameters;
    uint32_t numParameters;
    struct qak_ast_node *returnType;
    struct qak_ast_node *statements;
    uint32_t numStatements;
} qak_ast_function;

typedef struct qak_ast_variable {
    qak_span name;
    struct qak_ast_node *typeSpecifier;
    struct qak_ast_node *initializerExpression;
} qak_ast_variable;

typedef struct qak_ast_while {
    struct qak_ast_node *condition;
    struct qak_ast_node *statements;
    uint32_t numStatements;
} qak_ast_while;

typedef struct qak_ast_if {
    struct qak_ast_node *condition;
    struct qak_ast_node *trueBlock;
    uint32_t numTrueBlockStatements;
    struct qak_ast_node *falseBlock;
    uint32_t numFalseBlockStatements;
} qak_ast_if;

typedef struct qak_ast_return {
    struct qak_ast_node *returnValue;
} qak_ast_return;

typedef struct qak_ast_ternary_operation {
    struct qak_ast_node *condition;
    struct qak_ast_node *trueValue;
    struct qak_ast_node *falseValue;
} qak_ast_ternary_operation;

typedef struct qak_ast_binary_operation {
    qak_span op;
    qak_token_type opType;
    struct qak_ast_node *left;
    struct qak_ast_node *right;
} qak_ast_binary_operation;

typedef struct qak_ast_unary_operation {
    qak_span op;
    qak_token_type opType;
    struct qak_ast_node *value;
} qak_ast_unary_operation;

typedef struct qak_ast_literal {
    qak_token_type type;
    qak_span value;
} qak_ast_literal;

typedef struct qak_ast_variable_access {
    qak_span name;
} qak_ast_variable_access;

typedef struct qak_ast_function_call {
    struct qak_ast_node *variableAccess;
    struct qak_ast_node *arguments;
    uint32_t numArguments;
} qak_ast_function_call;

typedef struct qak_ast_module {
    qak_span name;
    struct qak_ast_node *functions;
    uint32_t numFunctions;
    struct qak_ast_node *statements;
    uint32_t numStatements;
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
    struct qak_ast_node *next;
} qak_ast_node;

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

typedef struct qak_module {
    qak_source *source;
    qak_allocator mem;
    qak_allocator bumpMem;
    qak_array_token *tokens;
    qak_errors errors;
    struct qak_ast_node *ast;
} qak_module;

/** Compiler **/
qak_module *qak_compiler_compile_file(const char *fileName);

qak_module *qak_compiler_compile_source(const char *fileName, const char *source);

/** Module **/
void qak_module_delete(qak_module *module);

void qak_module_get_source(qak_module *module, qak_source *source);

int qak_module_get_num_errors(qak_module *module);

void qak_module_get_error(qak_module *module, int errorIndex, qak_error *error);

void qak_module_print_errors(qak_module *module);

int qak_module_get_num_tokens(qak_module *module);

void qak_module_get_token(qak_module *module, int tokenIndex, qak_token *token);

void qak_module_print_tokens(qak_module *module);

struct qak_ast_node *qak_module_get_ast(qak_module *module);

void qak_module_print_ast(qak_ast_node *moduleAst);

#ifdef WASM
void qak_print_struct_offsets();
#endif

#ifdef __cplusplus
}
#endif
