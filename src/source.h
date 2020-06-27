

#ifndef QAK_SOURCE_H
#define QAK_SOURCE_H

#include "memory.h"
#include "array.h"

namespace qak {

    struct Line {
        uint32_t start;
        uint32_t end;
        uint32_t lineNumber;

        Line(uint32_t start, uint32_t end, uint32_t lineNumber) : start(start), end(end), lineNumber(lineNumber) {}

        uint32_t length() {
            return end - start;
        }
    };

    struct Source {
    private:
        Array<Line> _lines;

        void scanLines() {
            if (_lines.size() != 0) return;
            // BOZO Figure out the lines. This is very unfortunate, as we have to scan
            // the source twice. Once for lines, and a second time for tokens. However,
            // this will only be invoked if something requires a line, e.g. Errors.
            _lines.add(Line(0, 0, 0));
            uint32_t lineStart = 0;
            for (size_t i = 0; i < size; i++) {
                uint8_t c = data[i];
                if (c == '\n') {
                    _lines.add(Line(lineStart, i, _lines.size()));
                    lineStart = i + 1;
                }
            }

            if (_lines[_lines.size()].start != lineStart) {
                _lines.add(Line(lineStart, size - 1, _lines.size()));
            }
        }

    public:
        HeapAllocator &mem;
        const char *fileName;
        uint8_t *data;
        size_t size;

        Source(HeapAllocator &mem, const char *fileName, uint8_t *data, size_t size) : _lines(mem), mem(mem), fileName(fileName), data(data), size(size) {
        }

        ~Source() {
            if (data) {
                mem.free(data, __FILE__, __LINE__);
                data = nullptr;
            }
        }

        Array<Line> &lines() {
            scanLines();
            return _lines;
        }
    };

    struct Span {
        Source &source;
        uint32_t start;
        uint32_t startLine;
        uint32_t end;
        uint32_t endLine;

        Span(Source &source, uint32_t start, uint32_t startLine, uint32_t end, uint32_t endLine) : source(source), start(start), startLine(startLine), end(end),
                                                                                                   endLine(endLine) {}

        Span(Source &source, Span &start, Span &end) : source(source), start(start.start), startLine(start.startLine), end(end.start), endLine(end.endLine) {}

        const char *toCString(HeapAllocator &mem) {
            uint8_t *sourceData = source.data;
            uint32_t size = end - start + 1;
            uint8_t *cString = mem.alloc<uint8_t>(size, __FILE__, __LINE__);
            memcpy(cString, sourceData + start, size - 1);
            cString[size - 1] = 0;
            return (const char *) cString;
        }

        /** Returns whether the span text matches the needle. **/
        QAK_FORCE_INLINE bool match(const char *needle, uint32_t len) {
            if (end - start != len) return false;

            const uint8_t *sourceData = source.data + start;
            for (uint32_t i = 0; i < len; i++) {
                if (sourceData[i] != needle[i]) return false;
            }
            return true;
        }

        uint32_t length() {
            return end - start;
        }
    };
}

#endif //QAK_SOURCE_H
