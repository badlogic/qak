

#ifndef QAK_SOURCE_H
#define QAK_SOURCE_H

#include "memory.h"

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
}

#endif //QAK_SOURCE_H
