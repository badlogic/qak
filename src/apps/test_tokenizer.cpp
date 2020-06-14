#include <stdio.h>
#include "qak.h"

#include "test.h"

using namespace qak;

int main() {
    MemoryArea mem;
    Buffer file = qak::readFile("data/tokens.qak", mem);
    QAK_CHECK(file.data != nullptr, "Couldn't read test file data/tokens.qak");

    Array<Token> tokens(mem);
    Array<Error> errors(mem);
    tokenize({file, "data/tokens.qak"}, tokens, errors);

    for (u4 i = 0; i < tokens.getSize(); i++) {
        Token &token = tokens[i];
        printf("%s: %s\n", tokenTypeToString(token.type), token.toCString(mem));
    }

    if (errors.getSize()) {
        for (u4 i = 0; i < errors.getSize(); i++) {
            printf("Error: %s\n", errors[i].message);
        }
        return -1;
    }

    return 0;
}
