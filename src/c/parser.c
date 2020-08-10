#include "parser.h"
#include "io.h"
#include "tokenizer.h"
#include "error.h"

QAK_INLINE void token_stream_init(qak_allocator *allocator, qak_token_stream *stream, qak_source *source, qak_array_token *tokens, qak_array_error *errors) {
    stream->allocator = allocator;
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

/* Returnst he next token or null, without advancing the stream. */
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
            qak_array_line *lines = qak_source_get_lines(stream->source);
            qak_line *lastLine = &lines->items[lines->size - 1];
            qak_span span = (qak_span) {lastLine->data, lastLine->lineNumber, lastLine->lineNumber};
            const char* typeString = qak_token_type_to_string(type);
            qak_error error = qak_error_from_string(stream->allocator, &span, "Expected '%s', but reached the end of the source.", typeString);
            qak_array_error_add(stream->errors, error);
        } else {
            const char* typeString = qak_token_type_to_string(type);
            qak_error error = qak_error_from_string(stream->allocator, &lastToken->span, "Expected '%s', but got '%.*s'", typeString,
                                                    lastToken->span.data.length, lastToken->span.data.data);
            qak_array_error_add(stream->errors, error);
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
            qak_array_line *lines = qak_source_get_lines(stream->source);
            qak_line *lastLine = &lines->items[lines->size - 1];
            qak_span span = (qak_span) {lastLine->data, lastLine->lineNumber, lastLine->lineNumber};
            qak_error error = qak_error_from_string(stream->allocator, &span, "Expected '%.*s', but reached the end of the source.", len, text);
            qak_array_error_add(stream->errors, error);
        } else {
            qak_error error = qak_error_from_string(stream->allocator, &lastToken->span, "Expected '%.*s', but got '%.*s'", len, text,
                                                    lastToken->span.data.length, lastToken->span.data.data);
            qak_array_error_add(stream->errors, error);
        }
        return NULL;
    } else {
        return &stream->tokens->items[stream->index - 1];
    }
}

void qak_parser_init(qak_allocator *allocator, qak_parser *parser) {
    parser->allocator = allocator;
    parser->nodes = qak_array_ast_node_new(allocator, 16);
    parser->tokens = qak_array_token_new(allocator, 16);
}

void qak_parser_shutdown(qak_parser *parser) {
    qak_array_ast_node_delete(parser->nodes);
    qak_array_token_delete(parser->tokens);
}

qak_ast_node *parse_module(qak_parser *parser) {
    qak_token *moduleKeyword = expect_string(&parser->stream, QAK_STR("module"));
    if (!moduleKeyword) return NULL;

    qak_token *moduleName = expect(&parser->stream, QakTokenIdentifier);
    if (!moduleName) return NULL;

    qak_array_ast_node_set_size(parser->nodes, parser->nodes->size + 1);
    qak_ast_node *module = &parser->nodes->items[parser->nodes->size - 1];
    module->type = QakAstModule;
    module->span = moduleName->span;
    module->data.module.name = moduleName->span;
    return module;
}

qak_ast_node *qak_parse(qak_parser *parser, qak_source *source, qak_array_error *errors) {
    parser->source = source;
    parser->errors = errors;

    qak_array_token_clear(parser->tokens);
    qak_tokenize(source, parser->tokens, errors);
    if (errors->size) return NULL;
    token_stream_init(parser->allocator, &parser->stream, source, parser->tokens, errors);

    qak_ast_node *module = parse_module(parser);
    if (!module) return NULL;

    return module;
}
