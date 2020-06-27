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

        Error(Span span, const char *message) : span(span), message(message) {}

        Line getLine() {
            if (span.start == 0 && span.end == 0) {
                return {span.source, 0, 0, 1};
            }

            int32_t lineStart = span.start;
            const uint8_t *sourceData = span.source.data;
            while (true) {
                if (lineStart < 0) break;
                uint8_t c = sourceData[lineStart];
                if (c == '\n') {
                    lineStart = lineStart + 1;
                    break;
                }
                lineStart--;
            }
            if (lineStart < 0) lineStart = 0;

            int32_t lineEnd = span.end;
            while (true) {
                if (lineEnd > (int32_t) span.source.size - 1) break;
                uint8_t c = sourceData[lineEnd];
                if (c == '\n') {
                    break;
                }
                lineEnd++;
            }

            uint32_t lineNumber = 0;
            uint32_t idx = lineStart;
            while (idx > 0) {
                char c = sourceData[idx];
                if (c == '\n') {
                    lineNumber++;
                }
                idx--;
            }
            lineNumber++;

            return {span.source, (uint32_t) lineStart, (uint32_t) lineEnd, lineNumber};
        }

        void print() {
            HeapAllocator mem;
            Line line = getLine();
            uint8_t *lineStr = lineStr = mem.alloc<uint8_t>(line.getLength() + 1, __FILE__, __LINE__);
            if (line.getLength() > 0) memcpy(lineStr, line.source.data + line.start, line.getLength());
            lineStr[line.getLength()] = 0;

            printf("Error (%s:%i): %s\n", span.source.fileName, line.line, message);

            if (line.getLength() > 0) {
                printf("%s\n", lineStr);
                int32_t errorStart = span.start - line.start;
                int32_t errorEnd = errorStart + span.getLength() - 1;
                for (int32_t i = 0, n = line.getLength(); i < n; i++) {
                    bool useTab = (line.source.data + line.start)[i] == '\t';
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
