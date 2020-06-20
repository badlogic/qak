#include <cstdio>
#include <sys/time.h>
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

u8 io::timeMillis() {
    timeval time;
    gettimeofday(&time, NULL);
    return u8(time.tv_sec) * 1000 + u8(time.tv_usec / 1000);
}