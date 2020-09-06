#include "parser.h"
#include "io.h"
#include "tokenizer.h"
#include "error.h"

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

QAK_INLINE void
token_stream_init(qak_token_stream *stream, qak_source *source, qak_array_token *tokens, qak_errors *errors) {
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
            qak_errors_add(stream->errors, stream->source, span, "Expected '%s', but reached the end of the source.",
                           typeString);
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
            qak_errors_add(stream->errors, stream->source, span, "Expected '%.*s', but reached the end of the source.",
                           len, text);
        } else {
            qak_errors_add(stream->errors, stream->source, lastToken->span, "Expected '%.*s', but got '%.*s'", len,
                           text,
                           lastToken->span.data.length, lastToken->span.data.data);
        }
        return NULL;
    } else {
        return &stream->tokens->items[stream->index - 1];
    }
}

QAK_INLINE qak_ast_node *new_ast_node(qak_parser *parser, qak_ast_type type, size_t size, qak_span *span) {
    qak_ast_node *node = (qak_ast_node *) QAK_ALLOCATE(parser->nodeAllocator, uint8_t, size);
    node->type = type;
    node->span = *span;
    node->next = NULL;
    return node;
}

QAK_INLINE qak_ast_node *new_ast_node_2(qak_parser *parser, qak_ast_type type, size_t size, qak_span *start, qak_span *end) {
    qak_ast_node *node = (qak_ast_node *) QAK_ALLOCATE(parser->nodeAllocator, uint8_t, size);
    node->type = type;
    node->span.data = start->data;
    node->span.data.length = end->data.data - start->data.data;
    node->span.startLine = start->startLine;
    node->span.endLine = end->endLine;
    node->next = NULL;
    return node;
}

qak_ast_node *parse_expression(qak_parser *parser);

qak_ast_node *parse_statement(qak_parser *parser);

qak_ast_type_specifier *parse_type_specifier(qak_parser *parser) {
    qak_token_stream *stream = &parser->stream;
    qak_token *name = expect(stream, QakTokenIdentifier);
    if (!name) return NULL;

    qak_ast_type_specifier *type = (qak_ast_type_specifier *) new_ast_node(parser, QakAstTypeSpecifier, sizeof(qak_ast_type_specifier), &name->span);
    type->name = name->span;
    return type;
}

qak_ast_node *parse_access_or_call(qak_parser *parser) {
    qak_token_stream *stream = &parser->stream;
    qak_token *name = expect(stream, QakTokenIdentifier);
    if (!name) return NULL;

    qak_ast_variable_access *result = (qak_ast_variable_access *) new_ast_node(parser, QakAstVariableAccess, sizeof(qak_ast_variable_access), &name->span);
    result->name = name->span;

    // If the next token is "(", we have a function call.
    if (match(stream, QakTokenLeftParenthesis, true)) {
        // BOZO
        /*ArrayPoolMonitor<Expression *> expressionArrays(_expressionArrayPool);
        Array<Expression *> *arguments = parseArguments(expressionArrays.obtain(QAK_SRC_LOC));
        if (!arguments) return nullptr;

        qak_token *closingParan = expect(stream, QakTokenRightParenthesis);
        if (!closingParan) return NULL;

        result = _bumpMem->allocObject<FunctionCall>(*_bumpMem, *name, *closingParan, result, *arguments);*/
    }
    return (qak_ast_node *) result;
}

qak_ast_node *parse_access_or_call_or_literal(qak_parser *parser) {
    qak_token_stream *stream = &parser->stream;
    if (!has_more(stream)) {
        if (parser->tokens->size > 0) {
            qak_token *token = &parser->tokens->items[parser->tokens->size - 1];
            qak_errors_add(parser->errors, parser->source, token->span,
                           "Expected a variable, field, array, function call, method call, or literal.");
        } else {
            qak_errors_add(parser->errors, parser->source, (qak_span) {{NULL, 0}, 0, 0},
                           "Expected a variable, field, array, function call, method call, or literal.");
        }
        return NULL;
    }

    qak_token_type tokenType = peek(stream)->type;

    switch (tokenType) {
        case QakTokenStringLiteral:
        case QakTokenBooleanLiteral:
        case QakTokenDoubleLiteral:
        case QakTokenFloatLiteral:
        case QakTokenByteLiteral:
        case QakTokenShortLiteral:
        case QakTokenIntegerLiteral:
        case QakTokenLongLiteral:
        case QakTokenCharacterLiteral:
        case QakTokenNothingLiteral: {
            qak_token *token = consume(stream);
            qak_ast_literal *literal = (qak_ast_literal *) new_ast_node(parser, QakAstLiteral, sizeof(qak_ast_literal), &token->span);
            literal->type = tokenType;
            literal->value = token->span;
            return (qak_ast_node *) literal;
        }

        case QakTokenIdentifier:
            return parse_access_or_call(parser);

        default:
            qak_errors_add(parser->errors, parser->source, peek(stream)->span,
                           "Expected a variable, field, array, function call, method call, or literal.");
            return NULL;
    }
}

static qak_token_type unaryOperators[] = {QakTokenNot,
                                          QakTokenPlus,
                                          QakTokenMinus,
                                          QakTokenUnknown};

qak_ast_node *parse_unary_operator(qak_parser *parser) {
    qak_token_stream *stream = &parser->stream;
    qak_token_type *op = unaryOperators;
    while (*op != QakTokenUnknown) {
        if (match(stream, *op, false)) break;
        op++;
    }
    if (*op != QakTokenUnknown) {
        qak_token *op = consume(stream);
        qak_ast_node *expression = parse_unary_operator(parser);
        if (!expression) return NULL;
        qak_ast_unary_operation *operation = (qak_ast_unary_operation *) new_ast_node(parser, QakAstUnaryOperation, sizeof(qak_ast_unary_operation), &op->span);
        operation->op = op->span;
        operation->opType = op->type;
        operation->value = expression;
        return (qak_ast_node *) operation;
    } else {
        if (match(stream, QakTokenLeftParenthesis, true)) {
            qak_ast_node *expression = parse_expression(parser);
            if (!expression) return NULL;
            if (!expect(stream, QakTokenRightParenthesis)) return NULL;
            return expression;
        } else {
            return parse_access_or_call_or_literal(parser);
        }
    }
    return NULL;
}

#define OPERATOR_NUM_GROUPS 6
static qak_token_type binaryOperators[OPERATOR_NUM_GROUPS][5] = {
        {QakTokenAssignment,   QakTokenUnknown},
        {QakTokenOr,           QakTokenAnd,       QakTokenXor,        QakTokenUnknown},
        {QakTokenEqual,        QakTokenNotEqual,  QakTokenUnknown},
        {QakTokenLess,         QakTokenLessEqual, QakTokenGreater,    QakTokenGreaterEqual, QakTokenUnknown},
        {QakTokenPlus,         QakTokenMinus,     QakTokenUnknown},
        {QakTokenForwardSlash, QakTokenAsterisk,  QakTokenPercentage, QakTokenUnknown}
};

qak_ast_node *parse_binary_operator(qak_parser *parser, uint32_t level) {
    uint32_t nextLevel = level + 1;

    qak_ast_node *left =
            nextLevel == OPERATOR_NUM_GROUPS ? parse_unary_operator(parser) : parse_binary_operator(parser, nextLevel);
    if (!left) return NULL;

    qak_token_stream *stream = &parser->stream;
    while (has_more(stream)) {
        qak_token_type *op = binaryOperators[level];
        while (*op != QakTokenUnknown) {
            if (match(stream, *op, false)) break;
            op++;
        }
        if (*op == QakTokenUnknown) break;

        qak_token *opToken = consume(stream);
        qak_ast_node *right =
                nextLevel == OPERATOR_NUM_GROUPS ? parse_unary_operator(parser) : parse_binary_operator(parser,
                                                                                                        nextLevel);
        if (right == NULL) return NULL;

        left = new_ast_node_2(parser, QakAstBinaryOperation, sizeof(qak_ast_binary_operation), &left->span, &right->span);
        qak_ast_binary_operation *binaryOp = (qak_ast_binary_operation *) left;
        binaryOp->op = opToken->span;
        binaryOp->opType = *op;
        binaryOp->left = left;
        binaryOp->right = right;
    }
    return left;
}

qak_ast_node *parse_ternary_operator(qak_parser *parser) {
    qak_token_stream *stream = &parser->stream;
    qak_ast_node *condition = parse_binary_operator(parser, 0);
    if (!condition) return NULL;

    if (match(stream, QakTokenQuestionMark, true)) {
        qak_ast_node *trueValue = parse_ternary_operator(parser);
        if (!trueValue) return NULL;
        if (!match(stream, QakTokenColon, true)) return NULL;
        qak_ast_node *falseValue = parse_ternary_operator(parser);
        if (!falseValue) return NULL;

        qak_ast_ternary_operation *ternary = (qak_ast_ternary_operation *) new_ast_node_2(parser, QakAstTernaryOperation, sizeof(qak_ast_ternary_operation),
                                                                                          &condition->span, &falseValue->span);
        ternary->condition = condition;
        ternary->trueValue = trueValue;
        ternary->falseValue = falseValue;
        return (qak_ast_node *) ternary;
    } else {
        return condition;
    }
}

qak_ast_node *parse_expression(qak_parser *parser) {
    return parse_ternary_operator(parser);
}

qak_ast_parameter *parse_parameter(qak_parser *parser) {
    qak_token_stream *stream = &parser->stream;

    qak_token *name = consume(stream);
    if (!expect(stream, QakTokenColon)) return NULL;

    qak_ast_type_specifier *type = parse_type_specifier(parser);
    if (!type) return NULL;

    qak_ast_parameter *parameter = (qak_ast_parameter *) new_ast_node_2(parser, QakAstParameter, sizeof(qak_ast_parameter), &name->span, &type->info.span);
    parameter->name = name->span;
    parameter->typeSpecifier = type;
    return parameter;
}

bool parse_parameters(qak_parser *parser, qak_ast_parameter **parametersHead, uint32_t *numParameters) {
    qak_token_stream *stream = &parser->stream;
    if (!expect(stream, QakTokenLeftParenthesis)) return false;

    qak_ast_parameter *lastParameter = NULL;
    while (match(stream, QakTokenIdentifier, false)) {
        qak_ast_parameter *parameter = parse_parameter(parser);
        if (!parameter) return false;

        if (!*parametersHead) {
            *parametersHead = parameter;
            lastParameter = parameter;
        } else {
            lastParameter->info.next = (qak_ast_node *) parameter;
            lastParameter = parameter;
        }
        (*numParameters)++;

        if (!match(stream, QakTokenComma, true)) break;
    }

    return expect(stream, QakTokenRightParenthesis);
}

qak_ast_function *parse_function(qak_parser *parser) {
    qak_token_stream *stream = &parser->stream;

    qak_token *name = expect(stream, QakTokenIdentifier);
    if (!name) return NULL;

    qak_ast_parameter *parametersHead = NULL;
    uint32_t numParameters = 0;
    if (!parse_parameters(parser, &parametersHead, &numParameters)) return NULL;

    qak_ast_type_specifier *returnType = NULL;
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

    qak_ast_function *function = (qak_ast_function *) new_ast_node(parser, QakAstFunction, sizeof(qak_ast_function), &name->span);
    function->name = name->span;
    function->returnType = returnType;
    function->parameters = parametersHead;
    function->numParameters = numParameters;
    function->statements = statementsHead;
    function->numStatements = numStatements;
    return function;
}

qak_ast_node *parse_variable(qak_parser *parser) {
    qak_token_stream *stream = &parser->stream;
    expect_string(stream, QAK_STR("var"));

    qak_token *name = expect(stream, QakTokenIdentifier);
    if (!name) return NULL;

    qak_ast_type_specifier *type = NULL;
    if (match(stream, QakTokenColon, true)) {
        type = parse_type_specifier(parser);
        if (!type) return NULL;
    }

    qak_ast_node *expression = NULL;
    if (match(stream, QakTokenAssignment, true)) {
        expression = parse_expression(parser);
        if (!expression) return NULL;
    }

    qak_ast_variable *variable = (qak_ast_variable *) new_ast_node(parser, QakAstVariable, sizeof(qak_ast_variable), &name->span);
    variable->name = name->span;
    variable->typeSpecifier = type;
    variable->initializerExpression = expression;
    return (qak_ast_node *) variable;
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

qak_ast_node *parse_statement(qak_parser *parser) {
    qak_token_stream *stream = &parser->stream;
    if (match_string(stream, QAK_STR("var"), false)) {
        return parse_variable(parser);
    } else if (match_string(stream, QAK_STR("while"), false)) {
        return parse_while(parser);
    } else if (match_string(stream, QAK_STR("if"), false)) {
        return parse_if(parser);
    } else if (match_string(stream, QAK_STR("return"), false)) {
        return parse_return(parser);
    } else {
        return parse_expression(parser);
    }
}

static qak_ast_node *parse_module(qak_parser *parser) {
    qak_token *moduleKeyword = expect_string(&parser->stream, QAK_STR("module"));
    if (!moduleKeyword) return NULL;

    qak_token *moduleName = expect(&parser->stream, QakTokenIdentifier);
    if (!moduleName) return NULL;

    qak_ast_module *module = (qak_ast_module *) new_ast_node(parser, QakAstModule, sizeof(qak_ast_module), &moduleName->span);
    module->name = moduleName->span;
    module->numStatements = 0;
    module->statements = NULL;
    module->numFunctions = 0;
    module->functions = NULL;

    return (qak_ast_node *) module;
}

qak_ast_module *qak_parse(qak_source *source, qak_array_token *tokens, qak_errors *errors, qak_allocator *nodeAllocator) {
    qak_parser parser;
    parser.source = source;
    parser.tokens = tokens;
    parser.errors = errors;
    parser.nodeAllocator = nodeAllocator;

    qak_array_token_clear(parser.tokens);
    qak_tokenize(source, parser.tokens, errors);
    if (errors->errors->size) return NULL;
    token_stream_init(&parser.stream, source, tokens, errors);

    qak_ast_module *module = (qak_ast_module *) parse_module(&parser);
    if (!module) return NULL;

    qak_token_stream *stream = &parser.stream;
    qak_ast_function *lastFunction = NULL;
    uint32_t numFunctions = 0;
    qak_ast_node *lastStatement = NULL;
    uint32_t numStatements = 0;
    while (has_more(stream)) {
        if (match_string(stream, QAK_STR("function"), true)) {
            qak_ast_function *function = parse_function(&parser);
            if (!function) return NULL;

            if (!module->functions) {
                module->functions = function;
                lastFunction = function;
            } else {
                lastFunction->info.next = (qak_ast_node *) function;
                lastFunction = function;
            }
            numFunctions++;
        } else {
            qak_ast_node *statement = parse_statement(&parser);
            if (!statement) return NULL;

            if (!module->statements) {
                module->statements = statement;
                lastStatement = statement;
            } else {
                lastStatement->next = statement;
                lastStatement = statement;
            }
            numStatements++;
        }
    }

    module->numFunctions = numFunctions;
    module->numStatements = numStatements;

    return module;
}
