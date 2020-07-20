#ifndef QAK_C_API_H
#define QAK_C_API_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void *qak_compiler;

typedef void *qak_module;

/** A UTF-8 string from a Source with length bytes. **/
typedef struct qak_string {
    const char *data;
    size_t length;
} qak_string;

/** A source file with a name **/
typedef struct qak_source {
    qak_string data;
    qak_string fileName;
} qak_source;

typedef struct qak_span {
    qak_string data;
    uint32_t start;
    uint32_t end;
    uint32_t startLine;
    uint32_t endLine;
} qak_span;

typedef struct qak_line {
    qak_source *source;
    qak_string data;
    uint32_t lineNumber;
} qak_line;

typedef enum qak_token_type {
    // Simple tokens, sorted by literal length. Longer literals first.
    // The list of simple tokens is terminated via LastSimpleTokenType.
    Period,
    Comma,
    Semicolon,
    Colon,
    Plus,
    Minus,
    Asterisk,
    ForwardSlash,
    Percentage,
    LeftParenthesis,
    RightParenthesis,
    LeftBracket,
    RightBracket,
    LeftCurly,
    RightCurly,
    LessEqual,
    GreaterEqual,
    NotEqual,
    Equal,
    Less,
    Greater,
    Assignment,
    And,
    Or,
    Xor,
    Not,
    Hash,
    QuestionMark,
    Unknown,

    // These don't have a literal representation
    BooleanLiteral,
    DoubleLiteral,
    FloatLiteral,
    LongLiteral,
    IntegerLiteral,
    ShortLiteral,
    ByteLiteral,
    CharacterLiteral,
    StringLiteral,
    NothingLiteral,
    Identifier
} qak_token_type;

typedef struct qak_token {
    qak_token_type type;
    qak_span span;
} qak_token;

typedef struct qak_error {
    qak_string errorMessage;
    qak_span span;
} qak_error;

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

int qak_module_get_num_tokens(qak_module module);

void qak_module_get_token(qak_module moduleHandle, int tokenIndex, qak_token *token);

void qak_module_print_errors(qak_module module);

void qak_module_print_tokens(qak_module module);

void qak_module_print_ast(qak_module module);

#ifdef WASM
void qak_print_struct_offsets();
#endif

#ifdef __cplusplus
}
#endif

#endif
