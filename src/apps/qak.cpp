#include <stdio.h>
#include "c/qak.h"

void printHelp() {
    printf("Usage: qak <file.qak>");
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printHelp();
        return 0;
    }

    return 0;
}
