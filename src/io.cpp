#include "io.h"

#include <cstdio>

#define SOKOL_IMPL

#include "3rdparty/sokol_time.h"


using namespace qak;


Source *io::readFile(const char *fileName, HeapAllocator &mem) {
    FILE *file = fopen(fileName, "rb");
    if (file == nullptr) {
        return nullptr;
    }

    // BOZO error handling
    fseek(file, 0L, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0L, SEEK_SET);

    uint8_t *data = mem.alloc<uint8_t>(size, QAK_SRC_LOC);
    fread(data, sizeof(uint8_t), size, file);
    fclose(file);

    size_t fileNameLength = strlen(fileName);
    char *fileNameCopy = mem.alloc<char>(fileNameLength, QAK_SRC_LOC);
    memcpy(fileNameCopy, fileName, fileNameLength);
    return mem.allocObject<Source>(QAK_SRC_LOC, mem, fileNameCopy, data, size);
}

static bool isTimeSetup = false;

double io::timeMillis() {
    if (!isTimeSetup) {
        stm_setup();
        isTimeSetup = true;
    }
    uint64_t time = stm_now();
    return stm_ms(time);
}