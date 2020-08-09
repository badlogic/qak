#include "c/qak.h"
#include "c/tokenizer.h"
#include "c/io.h"
#include "c/allocation.h"
#include "test.h"

void testTokenizer() {
    printf("========= Test: tokenizer\n");

    qak_allocator mem;
    qak_allocator_init(&mem);

    qak_source *source = qak_io_read_source_from_file(&mem, "data/tokens.qak");
    QAK_CHECK(source, "Couldn't read test file data/tokens.qak");

    qak_array_token *tokens = qak_array_token_new(&mem, 16);
    qak_array_error *errors = qak_array_error_new(&mem, 16);

    qak_tokenize(source, tokens, errors);
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

    qak_source_delete(source);
    qak_array_token_delete(tokens);
    qak_array_error_delete(errors);

    qak_allocator_print(&mem);
    printf("\n");
}

int main(int argc, char** argv) {
    testTokenizer();
    return 0;
}