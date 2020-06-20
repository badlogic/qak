#ifndef QAK_ERROR_H
#define QAK_ERROR_H

#include "source.h"
#include "array.h"
#include <cstdarg>

namespace qak {

    struct Error {
        Span span;
        const char *message;

        Error(Span span, const char *message) : span(span),  message(message) {}

        Line getLine() {
            if (span.start == 0 && span.end == 0) {
                return {span.source, 0, 0, 1};
            }

            s4 lineStart = span.start;
            const u1 *sourceData = span.source.buffer.data;
            while (true) {
                if (lineStart < 0) break;
                u1 c = sourceData[lineStart];
                if (c == '\n') {
                    lineStart = lineStart + 1;
                    break;
                }
                lineStart--;
            }
            if (lineStart < 0) lineStart = 0;

            s4 lineEnd = span.end;
            while (true) {
                if (lineEnd > (s4) span.source.buffer.size - 1) break;
                u1 c = sourceData[lineEnd];
                if (c == '\n') {
                    break;
                }
                lineEnd++;
            }

            u4 lineNumber = 0;
            u4 idx = lineStart;
            while (idx > 0) {
                char c = sourceData[idx];
                if (c == '\n') {
                    lineNumber++;
                }
                idx--;
            }
            lineNumber++;

            return {span.source, (u4) lineStart, (u4) lineEnd, lineNumber};
        }

        void print() {
            HeapAllocator mem;
            Line line = getLine();
            u1 *lineStr = lineStr = mem.alloc<u1>(line.getLength() + 1, __FILE__, __LINE__);
            if (line.getLength() > 0) memcpy(lineStr, line.source.buffer.data + line.start, line.getLength());
            lineStr[line.getLength()] = 0;

            printf("Error (%s:%i): %s\n", span.source.fileName, line.line, message);

            if (line.getLength() > 0) {
                printf("%s\n", lineStr);
                s4 errorStart = span.start - line.start;
                s4 errorEnd = errorStart + span.getLength();
                for (s4 i = 0, n = line.getLength(); i < n; i++) {
                    bool useTab = (line.source.buffer.data + line.start)[i] == '\t';
                    printf("%s", i >= errorStart && i <= errorEnd ? "^" : (useTab ? "\t" : " "));
                }
            }
        }
    };

    struct Errors {
        HeapAllocator &mem;
        Array<Error> errors;

        Errors(HeapAllocator &mem): mem(mem), errors(mem) {}

        void add(Error error) {
            errors.add(error);
        }

        void add(Span span, const char* msg...) {
            va_list args;
            va_start(args, msg);

            char* buffer = mem.alloc<char>(1024, __FILE__, __LINE__);
            int len = vsnprintf(buffer, 1024, msg, args);
            if (len > 1024) {
                mem.free(buffer, __FILE__, __LINE__);
                buffer = mem.alloc<char>(len + 1, __FILE__, __LINE__);
                vsnprintf(buffer, len + 1, msg, args);
            }
            va_end(args);
            add({span, buffer});
        }

        Array<Error> &getErrors() {
            return errors;
        }

        bool hasErrors() {
            return errors.getSize() != 0;
        }

        void print() {
            for (u4 i = 0; i < errors.getSize(); i++) {
                errors[i].print();
            }
            printf("\n");
            fflush(stdout);
        }
    };
}

#define ERROR(span, ...) { errors.add(span, __VA_ARGS__); return; }
#define ERROR_RET(span, value, ...) { errors.add(span, __VA_ARGS__); return value; }

#endif //QAK_ERROR_H
