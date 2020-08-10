#pragma once

#include "qak.h"

typedef struct qak_token_stream {
    qak_allocator *allocator;
    qak_source *source;
    qak_array_token *tokens;
    qak_errors *errors;
    size_t index;
} qak_token_stream;

typedef struct qak_parser {
    qak_allocator *allocator;
    qak_array_ast_node *nodes;
    qak_array_token *tokens;

    // Initialized on each call to qak_parse().
    qak_token_stream stream;
    qak_source *source;
    qak_errors *errors;
} qak_parser;

qak_parser qak_parser_init(qak_allocator *allocator);

void qak_parser_shutdown(qak_parser *parser);

qak_ast_node *qak_parse(qak_parser *parser, qak_source *source, qak_errors *errors);
