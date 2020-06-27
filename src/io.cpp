#include "io.h"

#include <cstdio>

#define SOKOL_IMPL

#include "3rdparty/sokol_time.h"


using namespace qak;

Source *io::readFile(const char *fileName, HeapAllocator &mem) {
    FILE *file = fopen(fileName, "rb");
    if (file == nullptr) {
        fclose(file);
        return nullptr;
    }

    // BOZO error handling
    fseek(file, 0L, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0L, SEEK_SET);

    uint8_t *data = mem.alloc<uint8_t>(size, __FILE__, __LINE__);
    fread(data, sizeof(uint8_t), size, file);
    fclose(file);
    return mem.allocObject<Source>(__FILE__, __LINE__, mem, fileName, data, size);
}

static bool isTimeSetup = false;

uint64_t io::timeMillis() {
    if (!isTimeSetup) {
        stm_setup();
        isTimeSetup = true;
    }
    uint64_t time = stm_now();
    return stm_ms(time);
}