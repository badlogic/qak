#include "parser.h"
#include "io.h"
#include "tokenizer.h"
#include "error.h"

QAK_ARRAY_IMPLEMENT_INLINE(qak_array_line, qak_line)

typedef struct qak_token_stream {
    qak_source *source;
    qak_array_token *tokens;
    qak_errors *errors;
    size_t index;
} qak_token_stream;

typedef struct qak_parser {
    qak_token_stream stream;
    qak_source *source;
    qak_errors *errors;
    qak_array_token *tokens;
    qak_allocator *nodeAllocator;
} qak_parser;

QAK_INLINE void token_stream_init(qak_token_stream *stream, qak_source *source, qak_array_token *tokens, qak_errors *errors) {
    stream->source = source;
    stream->tokens = tokens;
    stream->errors = errors;
    stream->index = 0;
}

/* Returns whether there are more tokens in the stream. */
QAK_INLINE bool has_more(qak_token_stream *stream) {
    return stream->index < (size_t) stream->tokens->size;
}

/* Consumes the next token and returns it. */
QAK_INLINE qak_token *consume(qak_token_stream *stream) {
    if (!has_more(stream)) return NULL;
    return &stream->tokens->items[stream->index++];
}

/* Returns the next token or null, without advancing the stream. */
QAK_INLINE qak_token *peek(qak_token_stream *stream) {
    if (!has_more(stream)) return NULL;
    return &stream->tokens->items[stream->index];
}

/* Matches and optionally consumes the next token in case of a match. Returns whether the token matched. */
QAK_INLINE bool match(qak_token_stream *stream, qak_token_type type, bool consume) {
    if (stream->index >= stream->tokens->size) return false;
    if (stream->tokens->items[stream->index].type == type) {
        if (consume) stream->index++;
        return true;
    }
    return false;
}

/* Matches and optionally consumes the next token in case of a match. Returns whether the token matched. */
QAK_INLINE bool match_string(qak_token_stream *stream, const char *text, uint32_t len, bool consume) {
    if (stream->index >= stream->tokens->size) return false;
    if (qak_span_matches(&stream->tokens->items[stream->index].span, text, len)) {
        if (consume) stream->index++;
        return true;
    }
    return false;
}

/* Checks if the next token has the give type and optionally consumes, or throws an error if the next token did not match the
 * type. */
QAK_INLINE qak_token *expect(qak_token_stream *stream, qak_token_type type) {
    bool result = match(stream, type, true);
    if (!result) {
        qak_token *lastToken = stream->index < stream->tokens->size ? &stream->tokens->items[stream->index] : NULL;

        if (lastToken == NULL) {
            qak_source_get_lines(stream->source);
            qak_line *lastLine = &stream->source->lines[stream->source->numLines - 1];
            qak_span span = (qak_span) {lastLine->data, lastLine->lineNumber, lastLine->lineNumber};
            const char *typeString = qak_token_type_to_string(type);
            qak_errors_add(stream->errors, stream->source, span, "Expected '%s', but reached the end of the source.", typeString);
        } else {
            const char *typeString = qak_token_type_to_string(type);
            qak_errors_add(stream->errors, stream->source, lastToken->span, "Expected '%s', but got '%.*s'", typeString,
                           lastToken->span.data.length, lastToken->span.data.data);
        }
        return NULL;
    } else {
        return &stream->tokens->items[stream->index - 1];
    }
}

/* Checks if the next token matches the given text and optionally consumes, or throws an error if the next token did not match
 * the text. */
QAK_INLINE qak_token *expect_string(qak_token_stream *stream, const char *text, uint32_t len) {
    bool result = match_string(stream, text, len, true);
    if (!result) {
        qak_token *lastToken = stream->index < stream->tokens->size ? &stream->tokens->items[stream->index] : NULL;

        if (lastToken == NULL) {
            qak_source_get_lines(stream->source);
            qak_line *lastLine = &stream->source->lines[stream->source->numLines - 1];
            qak_span span = (qak_span) {lastLine->data, lastLine->lineNumber, lastLine->lineNumber};
            qak_errors_add(stream->errors, stream->source, span, "Expected '%.*s', but reached the end of the source.", len, text);
        } else {
            qak_errors_add(stream->errors, stream->source, lastToken->span, "Expected '%.*s', but got '%.*s'", len, text,
                           lastToken->span.data.length, lastToken->span.data.data);
        }
        return NULL;
    } else {
        return &stream->tokens->items[stream->index - 1];
    }
}

QAK_INLINE qak_ast_node *new_ast_node(qak_parser *parser, qak_ast_type type, qak_span *span) {
    qak_ast_node *node = QAK_ALLOCATE(parser->nodeAllocator, qak_ast_node, 1);
    node->type = type;
    node->span = *span;
    node->next = NULL;
    return node;
}

QAK_INLINE qak_ast_node *new_ast_node_2(qak_parser *parser, qak_ast_type type, qak_span *start, qak_span *end) {
    qak_ast_node *node = QAK_ALLOCATE(parser->nodeAllocator, qak_ast_node, 1);
    node->type = type;
    node->span.data = start->data;
    node->span.data.length = end->data.data - start->data.data;
    node->span.startLine = start->startLine;
    node->span.endLine = end->endLine;
    node->next = NULL;
    return node;
}

qak_ast_node *parse_module(qak_parser *parser);

qak_ast_node *parse_function(qak_parser *parser);

qak_ast_node *parse_parameter(qak_parser *parser);

bool parse_parameters(qak_parser *parser, qak_ast_node **parametersHead, uint32_t *numParameters);

qak_ast_node *parse_type_specifier(qak_parser *parser);

qak_ast_node *parse_statement(qak_parser *parser);

qak_ast_node *parse_variable(qak_parser *parser);

qak_ast_node *parse_while(qak_parser *parser);

qak_ast_node *parse_if(qak_parser *parser);

qak_ast_node *parse_return(qak_parser *parser);

qak_ast_node *parse_expression(qak_parser *parser);

qak_ast_node *parse_module(qak_parser *parser) {
    qak_token *moduleKeyword = expect_string(&parser->stream, QAK_STR("module"));
    if (!moduleKeyword) return NULL;

    qak_token *moduleName = expect(&parser->stream, QakTokenIdentifier);
    if (!moduleName) return NULL;

    qak_ast_node *module = new_ast_node(parser, QakAstModule, &moduleName->span);
    module->data.module.name = moduleName->span;
    module->data.module.numStatements = 0;
    module->data.module.statements = NULL;
    module->data.module.numFunctions = 0;
    module->data.module.functions = NULL;

    return module;
}

qak_ast_node *parse_parameter(qak_parser *parser) {
    qak_token_stream *stream = &parser->stream;

    qak_token *name = consume(stream);
    if (!expect(stream, QakTokenColon)) return NULL;

    qak_ast_node *type = parse_type_specifier(parser);
    if (!type) return NULL;

    qak_ast_node *parameter = new_ast_node_2(parser, QakAstParameter, &name->span, &type->span);
    parameter->data.parameter.name = name->span;
    parameter->data.parameter.typeSpecifier = type;
    return parameter;
}

bool parse_parameters(qak_parser *parser, qak_ast_node **parametersHead, uint32_t *numParameters) {
    qak_token_stream *stream = &parser->stream;
    if (!expect(stream, QakTokenLeftParenthesis)) return false;

    qak_ast_node *lastParameter = NULL;
    while (match(stream, QakTokenIdentifier, false)) {
        qak_ast_node *parameter = parse_parameter(parser);
        if (!parameter) return false;

        if (!*parametersHead) {
            *parametersHead = parameter;
            lastParameter = parameter;
        } else {
            lastParameter->next = parameter;
            lastParameter = parameter;
        }
        (*numParameters)++;

        if (!match(stream, QakTokenComma, true)) break;
    }

    return expect(stream, QakTokenRightParenthesis);
}

qak_ast_node *parse_type_specifier(qak_parser *parser) {
    qak_token_stream *stream = &parser->stream;
    qak_token *name = expect(stream, QakTokenIdentifier);
    if (!name) return NULL;

    qak_ast_node *type = new_ast_node(parser, QakAstTypeSpecifier, &name->span);
    type->data.typeSpecifier.name = name->span;
    return type;
}

qak_ast_node *parse_function(qak_parser *parser) {
    qak_token_stream *stream = &parser->stream;

    qak_token *name = expect(stream, QakTokenIdentifier);
    if (!name) return NULL;

    qak_ast_node *parametersHead = NULL;
    uint32_t numParameters = 0;
    if (!parse_parameters(parser, &parametersHead, &numParameters)) return NULL;

    qak_ast_node *returnType = NULL;
    if (match(stream, QakTokenColon, true)) {
        returnType = parse_type_specifier(parser);
        if (!returnType) return NULL;
    }

    qak_ast_node *statementsHead = NULL;
    qak_ast_node *lastStatement = NULL;
    uint32_t numStatements = 0;
    while (has_more(stream) && !match_string(stream, QAK_STR("end"), false)) {
        qak_ast_node *statement = parse_statement(parser);
        if (!statement) return NULL;
        if (!statementsHead) {
            statementsHead = statement;
            lastStatement = statement;
        } else {
            lastStatement->next = statement;
            lastStatement = statement;
        }
        numStatements++;
    }

    if (!expect_string(stream, QAK_STR("end"))) return NULL;

    qak_ast_node *function = new_ast_node(parser, QakAstFunction, &name->span);
    function->data.function.name = name->span;
    function->data.function.returnType = returnType;
    function->data.function.parameters = parametersHead;
    function->data.function.numParameters = numParameters;
    function->data.function.statements = statementsHead;
    function->data.function.numStatements = numStatements;
    return function;
}

qak_ast_node *parse_variable(qak_parser *parser) {
    qak_token_stream *stream = &parser->stream;
    expect_string(stream,QAK_STR("var"));

    qak_token *name = expect(stream, QakTokenIdentifier);
    if (!name) return NULL;

    qak_ast_node *type = NULL;
    if (match(stream, QakTokenColon, true)) {
        type = parse_type_specifier(parser);
        if (!type) return NULL;
    }

    qak_ast_node *expression = NULL;
    if (match(stream, QakTokenAssignment, true)) {
        expression = parse_expression(parser);
        if (!expression) return NULL;
    }

    qak_ast_node *variable = new_ast_node(parser, QakAstVariable, &name->span);
    variable->data.variable.name = name->span;
    variable->data.variable.typeSpecifier = type;
    variable->data.variable.initializerExpression = expression;
    return variable;
}

qak_ast_node *parse_while(qak_parser *parser) {
    return NULL;
}

qak_ast_node *parse_if(qak_parser *parser) {
    return NULL;
}

qak_ast_node *parse_return(qak_parser *parser) {
    return NULL;
}

qak_ast_node *parse_expression(qak_parser *parser) {
    return NULL;
}

qak_ast_node *parse_statement(qak_parser *parser) {
    qak_token_stream *stream = &parser->stream;
    if (match_string(stream, QAK_STR("var"), false)) {
        return parse_variable(parser);
    } else if (match_string(stream, QAK_STR("while"), false)) {
        return parse_while(parser);
    } else if (match_string(stream, QAK_STR("if"), false)) {
        return parse_if(parser);
    } else if (match_string(stream,QAK_STR("return"), false)) {
        return parse_return(parser);
    } else {
        return parse_expression(parser);
    }
}

qak_ast_node *qak_parse(qak_source *source, qak_array_token *tokens, qak_errors *errors, qak_allocator *nodeAllocator) {
    qak_parser parser;
    parser.source = source;
    parser.tokens = tokens;
    parser.errors = errors;
    parser.nodeAllocator = nodeAllocator;
    token_stream_init(&parser.stream, source, tokens, errors);

    qak_array_token_clear(parser.tokens);
    qak_tokenize(source, parser.tokens, errors);
    if (errors->errors->size) return NULL;
    token_stream_init(&parser.stream, source, tokens, errors);

    qak_ast_node *module = parse_module(&parser);
    if (!module) return NULL;

    qak_token_stream *stream = &parser.stream;
    qak_ast_node* lastFunction = NULL;
    uint32_t numFunctions = 0;
    qak_ast_node* lastStatement = NULL;
    uint32_t numStatements = 0;
    while (has_more(stream)) {
        if (match_string(stream, QAK_STR("function"), true)) {
            qak_ast_node *function = parse_function(&parser);
            if (!function) return NULL;

            if (!module->data.module.functions) {
                module->data.module.functions = function;
                lastFunction = function;
            } else {
                lastFunction->next = function;
                lastFunction = function;
            }
            numFunctions++;
        } else {
            qak_ast_node *statement = parse_statement(&parser);
            if (!statement) return NULL;

            if (!module->data.module.statements) {
                module->data.module.statements = statement;
                lastStatement = statement;
            } else {
                lastStatement->next = statement;
                lastStatement = statement;
            }
            numStatements++;
        }
    }

    module->data.module.numFunctions = numFunctions;
    module->data.module.numStatements = numStatements;

    return module;
}
