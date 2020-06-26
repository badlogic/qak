#include <stdio.h>
#include "qak.h"
#include "test.h"

using namespace qak;

void testBench() {
    Test test("Tokenizer - Benchmark");
    u8 start = io::timeMillis();
    HeapAllocator mem;
    Buffer file = io::readFile("data/tokens.qak", mem);
    QAK_CHECK(file.data != nullptr, "Couldn't read test file data/tokens.qak");
    Source source(file, "data/tokens.qak");

    Array<Token> tokens(mem);
    Errors errors(mem);
    u4 iterations = 1000000;
    for (u4 i = 0; i < iterations; i++) {
        tokens.clear();
        tokenizer::tokenize(source, tokens, errors);
    }

    f8 time = (io::timeMillis() - start) / 1000.0;
    f8 throughput = (f8) file.size * iterations / time / 1024 / 1024;
    printf("File size: %llu bytes\n", file.size);
    printf("Took %f\n", time);
    printf("Throughput %f MB/s\n", throughput);
}

void testTokenizer() {
    Test test("Tokenizer - all token types");
    HeapAllocator mem;
    Buffer file = io::readFile("data/tokens.qak", mem);
    QAK_CHECK(file.data != nullptr, "Couldn't read test file data/tokens.qak");
    Source source = {file, "data/tokens.qak"};

    Array<Token> tokens(mem);
    Errors errors(mem);

    tokenizer::tokenize(source, tokens, errors);

    QAK_CHECK(tokens.size() == 42, "Expected 42 tokens, got %llu", tokens.size())
    QAK_CHECK(errors.getErrors().size() == 0, "Expected 0 errors, got %llu", errors.getErrors().size());

    for (u4 i = 0; i < tokens.size(); i++) {
        Token &token = tokens[i];
        printf("%s (%d:%d): %s\n", tokenizer::tokenTypeToString(token.type), token.span.start, token.span.end, token.toCString(mem));
    }
}

void testError() {
    Test test("Tokenizer - unknown token");
    HeapAllocator mem;
    Buffer file = io::readFile("data/tokens_error.qak", mem);
    QAK_CHECK(file.data != nullptr, "Couldn't read test file data/tokens_error.qak");
    Source source(file, "data/tokens_error.qak");

    Array<Token> tokens(mem);
    Errors errors(mem);

    tokenizer::tokenize(source, tokens, errors);
    QAK_CHECK(errors.getErrors().size() == 1, "Expected 1 error, got %llu", errors.getErrors().size());

    errors.getErrors()[0].print();
}

int main() {
    testBench();
    testTokenizer();
    testError();
    return 0;
}
