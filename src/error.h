#ifndef QAK_ERROR_H
#define QAK_ERROR_H

#include "memory.h"
#include "array.h"

namespace qak {

    struct Source {
        Buffer buffer;
        const char *fileName;

        Source(Buffer buffer, const char *fileName) : buffer(buffer), fileName(fileName) {}
    };

    struct Span {
        Source source;
        u4 start;
        u4 end;

        Span(Source source, u4 start, u4 end) : source(source), start(start), end(end) {}

        const char *toCString(HeapAllocator &mem) {
            u1 *sourceData = source.buffer.data;
            u4 size = end - start + 1;
            u1 *cString = mem.alloc<u1>(size, __FILE__, __LINE__);
            memcpy(cString, sourceData + start, size - 1);
            cString[size - 1] = 0;
            return (const char *) cString;
        }

        bool match(const char *str) {
            u1 *sourceData = source.buffer.data;
            for (u4 i = start, j = 0; i < end && str[j] != 0; i++, j++) {
                if (sourceData[i] != str[j]) return false;
            }
            return true;
        }

        u4 getLength() {
            return end - start;
        }
    };

    struct Line {
        Source source;
        u4 start;
        u4 end;
        u4 line;

        Line(Source source, u4 start, u4 end, u4 line) : source(source), start(start), end(end), line(line) {}

        u4 getLength() {
            return end - start;
        }
    };

    struct Error {
        const char *message;
        Span span;

        Error(const char *message, Span span) : message(message), span(span) {}

        Line getLine() {
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
                if (lineEnd > (s4)span.source.buffer.size - 1) break;
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
            u1 *lineStr = mem.alloc<u1>(line.getLength() + 1, __FILE__, __LINE__);
            memcpy(lineStr, line.source.buffer.data + line.start, line.getLength());
            lineStr[line.getLength()] = 0;

            printf("Error (%s:%i): %s\n\n", span.source.fileName, line.line, message);
            printf("%s\n", lineStr);

            s4 errorStart = span.start - line.start;
            s4 errorEnd = errorStart + span.getLength();
            for (s4 i = 0, n = line.getLength(); i < n; i++) {
                bool useTab = (line.source.buffer.data + line.start)[i] == '\t';
                printf("%s", i >= errorStart && i <= errorEnd ? "^" : (useTab ? "\t" : " "));
            }
        }
    };
}

#define ERROR(message, span) { errors.add({message, span}); return; }

#endif //QAK_ERROR_H
