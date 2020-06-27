

#ifndef QAK_SOURCE_H
#define QAK_SOURCE_H

#include "memory.h"

namespace qak {
    struct Source {
        HeapAllocator &mem;
        const char *fileName;
        uint8_t *data;
        size_t size;

        Source(HeapAllocator &mem, const char *fileName, uint8_t *data, size_t size) : mem(mem), fileName(fileName), data(data), size(size) {}

        ~Source() {
            if (data) {
                mem.free(data, __FILE__, __LINE__);
                data = nullptr;
            }
        }
    };

    struct Span {
        Source &source;
        uint32_t start;
        uint32_t end;

        Span(Source &source, uint32_t start, uint32_t end) : source(source), start(start), end(end) {}

        const char *toCString(HeapAllocator &mem) {
            uint8_t *sourceData = source.data;
            uint32_t size = end - start + 1;
            uint8_t *cString = mem.alloc<uint8_t>(size, __FILE__, __LINE__);
            memcpy(cString, sourceData + start, size - 1);
            cString[size - 1] = 0;
            return (const char *) cString;
        }

        bool match(const char *str) {
            uint8_t *sourceData = source.data;
            for (uint32_t i = start, j = 0; i < end && str[j] != 0; i++, j++) {
                if (sourceData[i] != str[j]) return false;
            }
            return true;
        }

        uint32_t getLength() {
            return end - start;
        }
    };

    struct Line {
        Source &source;
        uint32_t start;
        uint32_t end;
        uint32_t line;

        Line(Source &source, uint32_t start, uint32_t end, uint32_t line) : source(source), start(start), end(end), line(line) {}

        uint32_t getLength() {
            return end - start;
        }
    };
}

#endif //QAK_SOURCE_H
