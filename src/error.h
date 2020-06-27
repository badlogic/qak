#ifndef QAK_ERROR_H
#define QAK_ERROR_H

#include "source.h"
#include "array.h"
#include <cstdarg>

namespace qak {

#define QAK_ERROR(span, ...) { errors.add(span, __VA_ARGS__); return; }

    struct Error {
        Span span;
        const char *message;
        Array <Line> lines;

        Error(Span span, const char *message) : span(span), message(message), lines(span.source.mem) {}

        Line &getLine() {
            return span.source.lines()[span.startLine];
        }

        void print() {
            HeapAllocator mem;
            Line &line = getLine();
            uint8_t *lineStr = lineStr = mem.alloc<uint8_t>(line.length() + 1, __FILE__, __LINE__);
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
    };

    struct Errors {
        HeapAllocator &mem;
        Array <Error> errors;

        Errors(HeapAllocator &mem) : mem(mem), errors(mem) {}

        void add(Error error) {
            errors.add(error);
        }

        void add(Span span, const char *msg...) {
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

        Array <Error> &getErrors() {
            return errors;
        }

        bool hasErrors() {
            return errors.size() != 0;
        }

        void print() {
            for (uint32_t i = 0; i < errors.size(); i++) {
                errors[i].print();
            }
            printf("\n");
            fflush(stdout);
        }
    };
}

#endif //QAK_ERROR_H
