#include <stdio.h>
#include "qak.h"
#include "test.h"

using namespace qak;

void testBench() {
    u8 start = qak::timeMillis();
    HeapAllocator mem;
    Buffer file = qak::readFile("data/tokens.qak", mem);
    QAK_CHECK(file.data != nullptr, "Couldn't read test file data/tokens.qak");

    Array<Token> tokens(mem);
    Array<Error> errors(mem);
    u4 iterations = 1000000;
    for (u4 i = 0; i < iterations; i++) {
        tokens.clear();
        errors.clear();
        tokenize({file, "data/tokens.qak"}, tokens, errors);
    }

    f8 time = (qak::timeMillis() - start) / 1000.0;
    f8 throughput = (f8) file.size * iterations / time / 1024 / 1024;
    printf("File size: %llu bytes\n", file.size);
    printf("Took %f\n", time);
    printf("Throughput %f MB/s\n", throughput);
}

void testTokenizer() {
    HeapAllocator mem;
    Buffer file = qak::readFile("data/tokens.qak", mem);
    QAK_CHECK(file.data != nullptr, "Couldn't read test file data/tokens.qak");

    Array<Token> tokens(mem);
    Array<Error> errors(mem);
    Source source = {file, "data/tokens.qak"};
    tokenize(source, tokens, errors);

    QAK_CHECK(tokens.getSize() == 42, "Expected 42 tokens, got %llu", tokens.getSize())
    QAK_CHECK(errors.getSize() == 0, "Expected 0 errors, got %llu", errors.getSize());

    for (u4 i = 0; i < tokens.getSize(); i++) {
        Token &token = tokens[i];
        printf("%s (%d:%d): %s\n", tokenTypeToString(token.type), token.span.start, token.span.end, token.toCString(mem));
    }
}

void testError() {
    HeapAllocator mem;
    Buffer file = qak::readFile("data/tokens_error.qak", mem);
    QAK_CHECK(file.data != nullptr, "Couldn't read test file data/tokens_error.qak");

    Array<Token> tokens(mem);
    Array<Error> errors(mem);

    tokenize({file, "data/tokens_error.qak"}, tokens, errors);
    QAK_CHECK(errors.getSize() == 1, "Expected 1 error, got %llu", errors.getSize());

    errors[0].print();
}

int main() {
    testBench();
    testTokenizer();
    testError();
    return 0;
}
