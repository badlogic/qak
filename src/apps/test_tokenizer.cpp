#include <stdio.h>
#include "qak.h"
#include "test.h"

using namespace qak;

int main() {
    HeapAllocator mem;
    Buffer file = qak::readFile("data/tokens.qak", mem);
    QAK_CHECK(file.data != nullptr, "Couldn't read test file data/tokens.qak");

    Array<Token> tokens(mem);
    Array<Error> errors(mem);
    tokenize({file, "data/tokens.qak"}, tokens, errors);

    QAK_CHECK(tokens.getSize() == 42, "Expected 42 tokens, got %llu", tokens.getSize())
    QAK_CHECK(errors.getSize() == 0, "Expected 0 errors, got %llu", errors.getSize());

    for (u4 i = 0; i < tokens.getSize(); i++) {
        Token &token = tokens[i];
        printf("%s (%d:%d): %s\n", tokenTypeToString(token.type), token.span.start, token.span.end,  token.toCString(mem));
    }

    return 0;
}
