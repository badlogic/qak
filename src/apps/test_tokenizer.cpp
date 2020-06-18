#include <stdio.h>
#include "qak.h"
#include "test.h"

using namespace qak;

int main() {
    u8 start = qak::timeMillis();
    HeapAllocator mem;
    Buffer file = qak::readFile("data/tokens.qak", mem);
    QAK_CHECK(file.data != nullptr, "Couldn't read test file data/tokens.qak");

    Array<Token> tokens(mem);
    Array<Error> errors(mem);

    int iterations = 1000000;
    for (int i = 0; i < iterations; i++) {
        tokens.clear();
        errors.clear();
        tokenize({file, "data/tokens.qak"}, tokens, errors);

        QAK_CHECK(tokens.getSize() == 42, "Expected 42 tokens, got %llu", tokens.getSize())
        QAK_CHECK(errors.getSize() == 0, "Expected 0 errors, got %llu", errors.getSize());

        /*for (u4 i = 0; i < tokens.getSize(); i++) {
            Token &token = tokens[i];
            printf("%s (%d:%d): %s\n", tokenTypeToString(token.type), token.span.start, token.span.end, token.toCString(mem));
        }*/
    }
    double time = (qak::timeMillis() - start) / 1000.0;
    double throughput = (double)file.size * iterations / time / 1024 / 1024;
    printf("File size: %llu bytes\n", file.size);
    printf("Took %f\n", time);
    printf("Throughput %f MB/s\n", throughput);
    return 0;
}
