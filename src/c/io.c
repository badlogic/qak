#include "io.h"
#include "qak.h"
#include "allocation.h"

#define SOKOL_IMPL

#include "3rdparty/sokol_time.h"

#include <stdio.h>

QAK_ARRAY_IMPLEMENT_MINIMAL_INLINE(qak_array_line, qak_line)

qak_source *qak_io_read_source_from_file(qak_allocator *allocator, const char *fileName) {
    FILE *file = fopen(fileName, "rb");
    if (file == NULL) {
        return NULL;
    }

    // BOZO error handling
    fseek(file, 0L, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0L, SEEK_SET);

    uint8_t *data = QAK_ALLOCATE(allocator, uint8_t, size + 1);
    data[size] = 0;
    fread(data, sizeof(uint8_t), size, file);
    fclose(file);

    size_t fileNameLength = strlen(fileName) + 1;
    char *fileNameCopy = QAK_ALLOCATE(allocator, char, fileNameLength);
    memcpy(fileNameCopy, fileName, fileNameLength);
    qak_source *source = QAK_ALLOCATE(allocator, qak_source, 1);
    *source = (qak_source) {allocator, {(char *) data, size}, {fileNameCopy, fileNameLength - 1}, NULL, 0};
    return source;
}

qak_source *qak_io_read_source_from_memory(qak_allocator *allocator, const char *fileName, const char *sourceCode) {
    size_t size = strlen(sourceCode) + 1;
    uint8_t *data = QAK_ALLOCATE(allocator, uint8_t, size);
    data[size - 1] = 0;
    memcpy(data, sourceCode, size);

    size_t fileNameLength = strlen(fileName) + 1;
    char *fileNameCopy = QAK_ALLOCATE(allocator, char, fileNameLength);
    memcpy(fileNameCopy, fileName, fileNameLength);
    qak_source *source = QAK_ALLOCATE(allocator, qak_source, 1);
    *source = (qak_source) {allocator, {(char *) data, size - 1}, {fileNameCopy, fileNameLength - 1}, NULL, 0};
    return source;
}

void qak_source_delete(qak_source *source) {
    QAK_FREE(source->allocator, source->data.data);
    QAK_FREE(source->allocator, source->fileName.data);
    if (source->lines != NULL) QAK_FREE(source->allocator, source->lines);
    QAK_FREE(source->allocator, source);
}

void qak_source_get_lines(qak_source *source) {
    if (source->lines != NULL) return;

    qak_array_line *lines = qak_array_line_new(source->allocator, 16);
    qak_array_line_add(lines, (qak_line) {{0}, 0});

    uint32_t lineStart = 0;
    char *data = source->data.data;
    for (size_t i = 0, n = source->data.length; i < n; i++) {
        uint8_t c = data[i];
        if (c == '\n') {
            qak_line line = {{data + lineStart, i - lineStart}, (uint32_t) lines->size};
            qak_array_line_add(lines, line);
            lineStart = (uint32_t) i + 1;
        }
    }

    if (lineStart < source->data.length) {
        qak_array_line_add(lines, (qak_line) {{data + lineStart, source->data.length - lineStart}, lines->size});
    }

    source->lines = lines->items;
    source->numLines = lines->size;
    QAK_FREE(source->allocator, lines);
}

double qak_io_time_millis() {
    static bool isTimeSetup = false;
    if (!isTimeSetup) {
        stm_setup();
        isTimeSetup = true;
    }
    uint64_t time = stm_now();
    return stm_ms(time);
}
