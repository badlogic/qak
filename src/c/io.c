#include "io.h"
#include "qak.h"
#include "allocation.h"

#define SOKOL_IMPL

#include "3rdparty/sokol_time.h"

#include <stdio.h>

qak_source *qak_io_read_source_from_file(qak_allocator *allocator, const char *fileName) {
    FILE *file = fopen(fileName, "rb");
    if (file == NULL) {
        return NULL;
    }

    // BOZO error handling
    fseek(file, 0L, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0L, SEEK_SET);

    uint8_t *data = QAK_ALLOCATE(allocator, uint8_t, size);
    fread(data, sizeof(uint8_t), size, file);
    fclose(file);

    size_t fileNameLength = strlen(fileName) + 1;
    char *fileNameCopy = QAK_ALLOCATE(allocator, char, fileNameLength);
    memcpy(fileNameCopy, fileName, fileNameLength);
    qak_source *source = QAK_ALLOCATE(allocator, qak_source, 1);
    *source = (qak_source) {allocator, {(char *) data, size}, {fileNameCopy, fileNameLength - 1}};
    return source;
}

void qak_source_delete(qak_source *source) {
    QAK_FREE(source->allocator, source->data.data);
    QAK_FREE(source->allocator, source->fileName.data);
    QAK_FREE(source->allocator, source);
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
