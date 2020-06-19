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

    HeapAllocator mem;
    Buffer file = qak::readFile(argv[1], mem);
    if (file.data == nullptr) {
        printf("Error: couldn't read file %s\n", argv[1]);
        return -1;
    }

    Array<Token> tokens(mem);
    Array<Error> errors(mem);
    tokenize({file, argv[1]}, tokens, errors);

    return 0;
}
