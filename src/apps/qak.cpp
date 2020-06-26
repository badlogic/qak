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
    Buffer file = io::readFile(argv[1], mem);
    if (file.data == nullptr) {
        printf("Error: couldn't read file %s\n", argv[1]);
        return -1;
    }
    Source source(file, argv[1]);

    Array<Token> tokens(mem);
    Errors errors(mem);
    tokenizer::tokenize(source, tokens, errors);

    return 0;
}
