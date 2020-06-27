#include <stdio.h>
#include "io.h"
#include "tokenizer.h"
#include "test.h"

using namespace qak;

void testBench() {
    Test test("Tokenizer - Benchmark");
    uint64_t start = io::timeMillis();
    HeapAllocator mem;
    Source *source = io::readFile("data/tokens.qak", mem);
    QAK_CHECK(source != nullptr, "Couldn't read test file data/tokens.qak");

    Array<Token> tokens(mem);
    Errors errors(mem);
    uint32_t iterations = 1000000;
    for (uint32_t i = 0; i < iterations; i++) {
        tokens.clear();
        tokenizer::tokenize(*source, tokens, errors);
    }

    double time = (io::timeMillis() - start) / 1000.0;
    double throughput = (double) source->size * iterations / time / 1024 / 1024;
    printf("File size: %zu bytes\n", source->size);
    printf("Took %f\n", time);
    printf("Throughput %f MB/s\n", throughput);
}

void testTokenizer() {
    Test test("Tokenizer - all token types");
    HeapAllocator mem;
    Source *source = io::readFile("data/tokens.qak", mem);
    QAK_CHECK(source != nullptr, "Couldn't read test file data/tokens.qak");

    Array<Token> tokens(mem);
    Errors errors(mem);

    tokenizer::tokenize(*source, tokens, errors);

    QAK_CHECK(tokens.size() == 42, "Expected 42 tokens, got %zu", tokens.size())
    QAK_CHECK(errors.getErrors().size() == 0, "Expected 0 errors, got %zu", errors.getErrors().size());

    for (uint32_t i = 0; i < tokens.size(); i++) {
        Token &token = tokens[i];
        printf("%s (%d:%d:%d): %s\n", tokenizer::tokenTypeToString(token.type), token.startLine, token.start, token.end, token.toCString(mem));
    }
}

void testError() {
    Test test("Tokenizer - unknown token");
    HeapAllocator mem;
    Source *source = io::readFile("data/tokens_error.qak", mem);
    QAK_CHECK(source != nullptr, "Couldn't read test file data/tokens_error.qak");

    Array<Token> tokens(mem);
    Errors errors(mem);

    tokenizer::tokenize(*source, tokens, errors);
    QAK_CHECK(errors.getErrors().size() == 1, "Expected 1 error, got %zu", errors.getErrors().size());

    errors.getErrors()[0].print();
}

int main() {
    testBench();
    testTokenizer();
    testError();
    return 0;
}
