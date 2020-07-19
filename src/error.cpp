#include "error.h"
#include <cstdarg>
#include <cstdio>

using namespace qak;

Line &Error::getLine() {
    return span.source.lines()[span.startLine];
}

void Error::print() {
    HeapAllocator mem;
    Line &line = getLine();
    uint8_t *lineStr = mem.alloc<uint8_t>(line.length() + 1, QAK_SRC_LOC);
    if (line.length() > 0) memcpy(lineStr, span.source.data + line.start, line.length());
    lineStr[line.length()] = 0;

    printf("Error (%s:%i): %s\n", span.source.fileName, line.lineNumber, message);

    if (line.length() > 0) {
        printf("%s\n", lineStr);
        int32_t errorStart = span.start - line.start;
        int32_t errorEnd = errorStart + span.length() - 1;
        for (int32_t i = 0, n = line.length(); i < n; i++) {
            bool useTab = (span.source.data + line.start)[i] == '\t';
            printf("%s", i >= errorStart && i <= errorEnd ? "^" : (useTab ? "\t" : " "));
        }
    }
}

void Errors::add(Error error) {
    errors.add(error);
}

void Errors::add(Span span, const char *msg...) {
    va_list args;
    va_start(args, msg);
    char scratch[1];
    int len = vsnprintf(scratch, 1, msg, args);
    char *buffer = bumpMem.alloc<char>(len + 1);
    vsnprintf(buffer, len + 1, msg, args);
    va_end(args);
    add({span, buffer});
}

void Errors::addAll(Errors &errors) {
    for (size_t i = 0; i < errors.getErrors().size(); i++) {
        add(errors.getErrors()[i]);
    }
}

Array<Error> &Errors::getErrors() {
    return errors;
}

bool Errors::hasErrors() {
    return errors.size() != 0;
}

void Errors::print() {
    for (uint32_t i = 0; i < errors.size(); i++) {
        errors[i].print();
    }
    printf("\n");
    fflush(stdout);
}