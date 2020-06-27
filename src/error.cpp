#include "error.h"

using namespace qak;

Line &Error::getLine() {
    return span.source.lines()[span.startLine];
}

void Error::print() {
    HeapAllocator mem;
    Line &line = getLine();
    uint8_t *lineStr = mem.alloc<uint8_t>(line.length() + 1, __FILE__, __LINE__);
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

    char *buffer = mem.alloc<char>(1024, __FILE__, __LINE__);
    int len = vsnprintf(buffer, 1024, msg, args);
    if (len > 1024) {
        mem.free(buffer, __FILE__, __LINE__);
        buffer = mem.alloc<char>(len + 1, __FILE__, __LINE__);
        vsnprintf(buffer, len + 1, msg, args);
    }
    va_end(args);
    add({span, buffer});
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