

#ifndef QAK_SOURCE_H
#define QAK_SOURCE_H

#include "memory.h"
#include "array.h"

namespace qak {

    /* A line stores the start and end byte index of a line in a Source, as well as
     * the 1-based line number. A line ends at a \n or the end of the Source. The
     * end byte index is the index of the \n or the size of the Source in case the
     * end of the file was reached.
     *
     * See Source::lines() and Source::scanLines(). */
    struct Line {
        /* The offset of the first byte of the line in the Source. */
        uint32_t start;

        /* The offset of the last byte of the line in the Source, either point
         * at a \n or the byte after the last byte in the Source. */
        uint32_t end;

        /* The 1-based line number. */
        uint32_t lineNumber;

        Line(uint32_t start, uint32_t end, uint32_t lineNumber) : start(start), end(end), lineNumber(lineNumber) {}

        /* The length of the line in bytes, excluding the \n. */
        uint32_t length() {
            return end - start;
        }
    };

    /* A source stores the raw byte data of a source file along with the file name.
     * It can also returns the individual lines making up the source, see Source::lines().
     * The raw byte data is assumed to have been allocated with the HeapAllocator passed
     * to the source's constructor. The source owns the raw byte data and will free it
     * through the HeapAllocator upon destruction. */
    struct Source {
    private:
        Array<Line> _lines;

        /* Lines are only needed for error reporting, so we create them with this method
         * the first time Source::lines() is called. */
        void scanLines() {
            if (_lines.size() != 0) return;
            _lines.add(Line(0, 0, 0));
            uint32_t lineStart = 0;
            for (size_t i = 0; i < size; i++) {
                uint8_t c = data[i];
                if (c == '\n') {
                    _lines.add(Line(lineStart, (uint32_t) i, (uint32_t) _lines.size()));
                    lineStart = (uint32_t) i + 1;
                }
            }

            if (lineStart < size) {
                _lines.add(Line(lineStart, (uint32_t) size, (uint32_t) _lines.size()));
            }
        }

    public:
        /* The HeapAllocator managing the memory of the data. */
        HeapAllocator &mem;

        /* The name of the file of which the source stores the data. */
        const char *fileName;

        /* The raw data of the source file. May be nullptr. */
        uint8_t *data;

        /* The size of the data in bytes. */
        size_t size;

        Source(HeapAllocator &mem, const char *fileName, uint8_t *data, size_t size) : _lines(mem), mem(mem), fileName(fileName), data(data), size(size) {
        }

        ~Source() {
            if (fileName) {
                mem.free((void *) fileName, QAK_SRC_LOC);
                fileName = nullptr;
            }
            if (data) {
                mem.free(data, QAK_SRC_LOC);
                data = nullptr;
            }
        }

        /* Returns the Lines making up this source. Line indexing starts at 1.
         * The line at index 0 has no meaning. */
        Array<Line> &lines() {
            scanLines();
            return _lines;
        }

        /* Creates a new Source with the given file name and source code. The name and
         * source code are copied defensively.*/
        static Source *fromMemory(HeapAllocator &mem, const char *fileName, const char *sourceCode) {
            size_t dataLength = strlen(sourceCode);
            uint8_t *data = mem.alloc<uint8_t>(dataLength, QAK_SRC_LOC);
            memcpy(data, sourceCode, dataLength);

            size_t fileNameLength = strlen(fileName);
            char *fileNameCopy = mem.alloc<char>(fileNameLength, QAK_SRC_LOC);
            memcpy(fileNameCopy, fileName, fileNameLength);
            Source *source = mem.allocObject<Source>(QAK_SRC_LOC, mem, fileNameCopy, data, dataLength);
            return source;
        }
    };

    /* A span stores the location of a sequence of bytes in a Source. The location
     * is given as start and end byte offsets into the data of the Source, as well
     * as the start and end line number spanned by the byte sequence in the Source.
     * See Source::lines(). The actual byte data is maintained by the Source. */
    struct Span {
        /* The Source the span references */
        Source &source;

        /* The offset of the first byte of the span in the Source data. */
        uint32_t start;

        /* The line number of the line the first byte of the span is contained in. */
        uint32_t startLine;

        /* The offset of the last byte (exclusive) of the span in the Source data. */
        uint32_t end;

        /* The line number of the line the last byte of the span is contained in. */
        uint32_t endLine;

        Span(Source &source, uint32_t start, uint32_t startLine, uint32_t end, uint32_t endLine) :
                source(source),
                start(start),
                startLine(startLine),
                end(end),
                endLine(endLine) {}

        Span(Source &source, Span &start, Span &end) :
                source(source),
                start(start.start),
                startLine(start.startLine),
                end(end.start),
                endLine(end.endLine) {}

        /* Converts the span's bytes to a null terminated C-string. Uses
         * the provided HeapAllocator to allocate the memory for the C-string.
         * The caller is responsible of freeing the returned C-string via the
         * HeapAllocator, or let the HeapAllocator automatically free it when
         * it is destructed. */
        const char *toCString(HeapAllocator &mem) {
            uint8_t *sourceData = source.data;
            uint32_t size = end - start + 1;
            uint8_t *cString = mem.alloc<uint8_t>(size, QAK_SRC_LOC);
            memcpy(cString, sourceData + start, size - 1);
            cString[size - 1] = 0;
            return (const char *) cString;
        }

        /* Returns whether the bytes making up the span matches the needle. The length of the needle
         * is given in number of bytes. */
        QAK_FORCE_INLINE bool matches(const char *needle, uint32_t length) {
            if (end - start != length) return false;

            const uint8_t *sourceData = source.data + start;
            for (uint32_t i = 0; i < length; i++) {
                if (sourceData[i] != needle[i]) return false;
            }
            return true;
        }

        /* Returns the length of the span in bytes. */
        uint32_t length() {
            return end - start;
        }
    };
}

#endif //QAK_SOURCE_H
