#include "c/qak.h"
#include "c/io.h"
#include "c/tokenizer.h"
#include "c/error.h"
#include "test.h"
#include <string.h>

void testBench() {
    printf("========= Test: tokenizer benchmark\n");
    double start = qak_io_time_millis();
    qak_allocator mem = qak_heap_allocator_init();

    qak_source *source = qak_io_read_source_from_file(&mem, "data/parser_benchmark.qak");
    QAK_CHECK(source != NULL, "Couldn't read test file data/parser_benchmark.qak");

    qak_array_token *tokens = qak_array_token_new(&mem, 16);
    qak_errors errors = qak_errors_init(&mem);

    uint32_t iterations = 100000;
    for (uint32_t i = 0; i < iterations; i++) {
        qak_array_token_clear(tokens);
        qak_tokenize(source, tokens, &errors);
    }

    double time = (qak_io_time_millis() - start) / 1000.0;
    double throughput = (double) source->data.length * iterations / time / 1024 / 1024;
    printf("File size: %zu bytes\n", source->data.length);
    printf("Took %f\n", time);
    printf("Throughput %f MB/s\n", throughput);

    qak_allocator_shutdown(&mem);
    printf("SUCCESS\n");
}

void testTokenizer() {
    printf("========= Test: tokenizer\n");

    qak_allocator mem = qak_heap_allocator_init();

    qak_source *source = qak_io_read_source_from_file(&mem, "data/tokens.qak");
    QAK_CHECK(source, "Couldn't read test file data/tokens.qak");

    qak_array_token *tokens = qak_array_token_new(&mem, 16);
    qak_errors errors = qak_errors_init(&mem);

    qak_tokenize(source, tokens, &errors);
    QAK_CHECK(tokens->size == 42, "Expected 42 tokens, got %zu", tokens->size);

    int i = 0;
    QAK_CHECK(qak_span_matches(&tokens->items[i].span, QAK_STR("<=")), "Unexpected token.");
    QAK_CHECK(tokens->items[i++].type == QakTokenLessEqual, "Unexpected token type.");
    QAK_CHECK(qak_span_matches(&tokens->items[i].span, QAK_STR(">=")), "Unexpected token.");
    QAK_CHECK(tokens->items[i++].type == QakTokenGreaterEqual, "Unexpected token type.");
    QAK_CHECK(qak_span_matches(&tokens->items[i].span, QAK_STR("==")), "Unexpected token.");
    QAK_CHECK(tokens->items[i++].type == QakTokenEqual, "Unexpected token type.");
    QAK_CHECK(qak_span_matches(&tokens->items[i].span, QAK_STR("!=")), "Unexpected token.");
    QAK_CHECK(tokens->items[i++].type == QakTokenNotEqual, "Unexpected token type.");
    QAK_CHECK(qak_span_matches(&tokens->items[i].span, QAK_STR("<")), "Unexpected token.");
    QAK_CHECK(tokens->items[i++].type == QakTokenLess, "Unexpected token type.");
    QAK_CHECK(qak_span_matches(&tokens->items[i].span, QAK_STR(">")), "Unexpected token.");
    QAK_CHECK(tokens->items[i++].type == QakTokenGreater, "Unexpected token type.");
    QAK_CHECK(qak_span_matches(&tokens->items[i].span, QAK_STR("=")), "Unexpected token.");
    QAK_CHECK(tokens->items[i++].type == QakTokenAssignment, "Unexpected token type.");
    QAK_CHECK(qak_span_matches(&tokens->items[i].span, QAK_STR(".")), "Unexpected token.");
    QAK_CHECK(tokens->items[i++].type == QakTokenPeriod, "Unexpected token type.");
    QAK_CHECK(qak_span_matches(&tokens->items[i].span, QAK_STR(",")), "Unexpected token.");
    QAK_CHECK(tokens->items[i++].type == QakTokenComma, "Unexpected token type.");
    QAK_CHECK(qak_span_matches(&tokens->items[i].span, QAK_STR(";")), "Unexpected token.");
    QAK_CHECK(tokens->items[i++].type == QakTokenSemicolon, "Unexpected token type.");
    QAK_CHECK(qak_span_matches(&tokens->items[i].span, QAK_STR(":")), "Unexpected token.");
    QAK_CHECK(tokens->items[i++].type == QakTokenColon, "Unexpected token type.");
    QAK_CHECK(qak_span_matches(&tokens->items[i].span, QAK_STR("+")), "Unexpected token.");
    QAK_CHECK(tokens->items[i++].type == QakTokenPlus, "Unexpected token type.");
    QAK_CHECK(qak_span_matches(&tokens->items[i].span, QAK_STR("-")), "Unexpected token.");
    QAK_CHECK(tokens->items[i++].type == QakTokenMinus, "Unexpected token type.");
    QAK_CHECK(qak_span_matches(&tokens->items[i].span, QAK_STR("*")), "Unexpected token.");
    QAK_CHECK(tokens->items[i++].type == QakTokenAsterisk, "Unexpected token type.");
    QAK_CHECK(qak_span_matches(&tokens->items[i].span, QAK_STR("/")), "Unexpected token.");
    QAK_CHECK(tokens->items[i++].type == QakTokenForwardSlash, "Unexpected token type.");
    QAK_CHECK(qak_span_matches(&tokens->items[i].span, QAK_STR("%")), "Unexpected token.");
    QAK_CHECK(tokens->items[i++].type == QakTokenPercentage, "Unexpected token type.");
    QAK_CHECK(qak_span_matches(&tokens->items[i].span, QAK_STR("(")), "Unexpected token.");
    QAK_CHECK(tokens->items[i++].type == QakTokenLeftParenthesis, "Unexpected token type.");
    QAK_CHECK(qak_span_matches(&tokens->items[i].span, QAK_STR(")")), "Unexpected token.");
    QAK_CHECK(tokens->items[i++].type == QakTokenRightParenthesis, "Unexpected token type.");
    QAK_CHECK(qak_span_matches(&tokens->items[i].span, QAK_STR("[")), "Unexpected token.");
    QAK_CHECK(tokens->items[i++].type == QakTokenLeftBracket, "Unexpected token type.");
    QAK_CHECK(qak_span_matches(&tokens->items[i].span, QAK_STR("]")), "Unexpected token.");
    QAK_CHECK(tokens->items[i++].type == QakTokenRightBracket, "Unexpected token type.");
    QAK_CHECK(qak_span_matches(&tokens->items[i].span, QAK_STR("{")), "Unexpected token.");
    QAK_CHECK(tokens->items[i++].type == QakTokenLeftCurly, "Unexpected token type.");
    QAK_CHECK(qak_span_matches(&tokens->items[i].span, QAK_STR("}")), "Unexpected token.");
    QAK_CHECK(tokens->items[i++].type == QakTokenRightCurly, "Unexpected token type.");
    QAK_CHECK(qak_span_matches(&tokens->items[i].span, QAK_STR("&")), "Unexpected token.");
    QAK_CHECK(tokens->items[i++].type == QakTokenAnd, "Unexpected token type.");
    QAK_CHECK(qak_span_matches(&tokens->items[i].span, QAK_STR("|")), "Unexpected token.");
    QAK_CHECK(tokens->items[i++].type == QakTokenOr, "Unexpected token type.");
    QAK_CHECK(qak_span_matches(&tokens->items[i].span, QAK_STR("^")), "Unexpected token.");
    QAK_CHECK(tokens->items[i++].type == QakTokenXor, "Unexpected token type.");
    QAK_CHECK(qak_span_matches(&tokens->items[i].span, QAK_STR("!")), "Unexpected token.");
    QAK_CHECK(tokens->items[i++].type == QakTokenNot, "Unexpected token type.");
    QAK_CHECK(qak_span_matches(&tokens->items[i].span, QAK_STR("?")), "Unexpected token.");
    QAK_CHECK(tokens->items[i++].type == QakTokenQuestionMark, "Unexpected token type.");
    QAK_CHECK(qak_span_matches(&tokens->items[i].span, QAK_STR("한자🥴")), "Unexpected token.");
    QAK_CHECK(tokens->items[i++].type == QakTokenIdentifier, "Unexpected token type.");
    QAK_CHECK(qak_span_matches(&tokens->items[i].span, QAK_STR("123")), "Unexpected token.");
    QAK_CHECK(tokens->items[i++].type == QakTokenIntegerLiteral, "Unexpected token type.");
    QAK_CHECK(qak_span_matches(&tokens->items[i].span, QAK_STR("123b")), "Unexpected token.");
    QAK_CHECK(tokens->items[i++].type == QakTokenByteLiteral, "Unexpected token type.");
    QAK_CHECK(qak_span_matches(&tokens->items[i].span, QAK_STR("123s")), "Unexpected token.");
    QAK_CHECK(tokens->items[i++].type == QakTokenShortLiteral, "Unexpected token type.");
    QAK_CHECK(qak_span_matches(&tokens->items[i].span, QAK_STR("123l")), "Unexpected token.");
    QAK_CHECK(tokens->items[i++].type == QakTokenLongLiteral, "Unexpected token type.");
    QAK_CHECK(qak_span_matches(&tokens->items[i].span, QAK_STR("123.2")), "Unexpected token.");
    QAK_CHECK(tokens->items[i++].type == QakTokenFloatLiteral, "Unexpected token type.");
    QAK_CHECK(qak_span_matches(&tokens->items[i].span, QAK_STR("123.3f")), "Unexpected token.");
    QAK_CHECK(tokens->items[i++].type == QakTokenFloatLiteral, "Unexpected token type.");
    QAK_CHECK(qak_span_matches(&tokens->items[i].span, QAK_STR("123.4d")), "Unexpected token.");
    QAK_CHECK(tokens->items[i++].type == QakTokenDoubleLiteral, "Unexpected token type.");
    QAK_CHECK(qak_span_matches(&tokens->items[i].span, QAK_STR("'c'")), "Unexpected token.");
    QAK_CHECK(tokens->items[i++].type == QakTokenCharacterLiteral, "Unexpected token type.");
    QAK_CHECK(qak_span_matches(&tokens->items[i].span, QAK_STR("'\\n'")), "Unexpected token.");
    QAK_CHECK(tokens->items[i++].type == QakTokenCharacterLiteral, "Unexpected token type.");
    QAK_CHECK(qak_span_matches(&tokens->items[i].span, QAK_STR("true")), "Unexpected token.");
    QAK_CHECK(tokens->items[i++].type == QakTokenBooleanLiteral, "Unexpected token type.");
    QAK_CHECK(qak_span_matches(&tokens->items[i].span, QAK_STR("false")), "Unexpected token.");
    QAK_CHECK(tokens->items[i++].type == QakTokenBooleanLiteral, "Unexpected token type.");
    QAK_CHECK(qak_span_matches(&tokens->items[i].span, QAK_STR("nothing")), "Unexpected token.");
    QAK_CHECK(tokens->items[i++].type == QakTokenNothingLiteral, "Unexpected token type.");
    QAK_CHECK(qak_span_matches(&tokens->items[i].span, QAK_STR("_Some987Identifier")), "Unexpected token.");
    QAK_CHECK(tokens->items[i++].type == QakTokenIdentifier, "Unexpected token type.");
    QAK_CHECK(qak_span_matches(&tokens->items[i].span, QAK_STR("\"Hello world. 한자🥴\"")), "Unexpected token.");
    QAK_CHECK(tokens->items[i++].type == QakTokenStringLiteral, "Unexpected token type.");

    qak_allocator_shutdown(&mem);
    printf("SUCCESS\n");
}

void testErrors() {
    printf("========= Test: tokenizer\n");
    qak_allocator mem = qak_heap_allocator_init();

    qak_source *source = qak_io_read_source_from_file(&mem, "data/tokens_error.qak");
    QAK_CHECK(source, "Couldn't read test file data/tokens.qak");

    qak_array_token *tokens = qak_array_token_new(&mem, 16);
    qak_errors errors = qak_errors_init(&mem);

    qak_tokenize(source, tokens, &errors);
    QAK_CHECK(errors.errors->size == 1, "Expected 1 error, got %zu", errors.errors->size);

    qak_error_print(&errors.errors->items[0]);

    qak_allocator_shutdown(&mem);
    printf("SUCCESS\n");
}

QAK_ARRAY_DECLARE(qak_array_token_type, qak_token_type)

QAK_ARRAY_IMPLEMENT(qak_array_token_type, qak_token_type)

/* Use this to generate the lookup table in tokenizer.c in case we add new qak_token_types. */
void generateLiteralToTokenArray() {
    qak_allocator mem = qak_heap_allocator_init();
    qak_token_type type = QakTokenPeriod;
    qak_array_token_type *types = qak_array_token_type_new(&mem, 256);
    qak_array_token_type_set_size(types, 256);
    for (int i = 0; i < 256; i++) types->items[i] = QakTokenUnknown;

    // Else check for simple tokens
    while (type != QakTokenUnknown) {
        const char *literal = qak_token_type_to_string((qak_token_type) type);
        if (strlen(literal) == 1) {
            types->items[(size_t) literal[0]] = (qak_token_type) type;
        } else {
            printf("Non single-character literal: %s\n", literal);
        }
        type++;
    }

    printf("static const uint32_t literalToTokenType[] = {\n");
    for (int i = 0; i < (int) types->size; i++) {
        printf("%d", types->items[i]);
        if (i < (int) types->size - 1)
            if (types->items[i] != QakTokenUnknown)
                printf(" /* %s */, ", qak_token_type_to_string(types->items[i]));
            else
                printf(", ");
        else
            printf("\n");
    }
    printf("};");
    printf("\n");

    qak_allocator_shutdown(&mem);
    printf("\n");
}

int main(int argc, char **argv) {
    QAK_UNUSED(argc);
    QAK_UNUSED(argv);

    generateLiteralToTokenArray();
    testTokenizer();
    testErrors();
    testBench();
    return 0;
}
