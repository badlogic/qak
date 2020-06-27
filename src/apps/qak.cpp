#include <stdio.h>
#include "qak_c_api.h"

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
