#include <stdio.h>
#include "qak.h"

using namespace qak;

void printHelp() {
    printf("Usage: qak <file.qak>");
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printHelp();
        return 0;
    }

    MemoryArea mem;
    Buffer file = qak::readFile(argv[1], mem);

    Array<Token> tokens(mem);
    Array<Error> errors(mem);
    tokenize({file, argv[1]}, tokens, errors);

    for (int i = 0; i < tokens.getSize(); i++) {
        Token &token = tokens[i];
        printf("%s: %s\n", tokenTypeToString(token.type), token.toCString(mem));
    }

    if (errors.getSize()) {
        for (int i = 0; i < errors.getSize(); i++) {
            printf("Error: %s\n", errors[i].message);
        }
    }

    return 0;
}
