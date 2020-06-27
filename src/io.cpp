#include <cstdio>
#define SOKOL_IMPL
#include "3rdparty/sokol_time.h"
#include "io.h"

using namespace qak;

Buffer io::readFile(const char *fileName, HeapAllocator &mem) {
    FILE *file = fopen(fileName, "rb");
    if (file == nullptr) {
        fclose(file);
        return {mem, nullptr, 0};
    }

    fseek(file, 0L, SEEK_END);
    u8 size = ftell(file);
    fseek(file, 0L, SEEK_SET);

    u1 *content = mem.alloc<u1>(size, __FILE__, __LINE__);
    if (content == nullptr) {
        fclose(file);
        return {mem, nullptr, 0};
    }
    fread(content, sizeof(u1), size, file);
    fclose(file);
    return {mem, content, size};
}

static bool isTimeSetup = false;

u8 io::timeMillis() {
    if (!isTimeSetup) {
        stm_setup();
        isTimeSetup = true;
    }
    uint64_t time = stm_now();
    return stm_ms(time);
}