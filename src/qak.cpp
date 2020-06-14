#include <stdio.h>
#include "qak.h"

using namespace qak;

void printHelp() {
    printf("Usage: qak <file.qak>");
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printHelp();
        return 0;
    }

    {
        MemoryArea mem;
        Buffer file = qak::readFile(argv[1], mem);
        mem.printAllocations();
        file.free();
        mem.printAllocations();
    }

    return 0;
}
