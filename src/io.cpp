#include <cstdio>
#include "io.h"

using namespace qak;

Buffer qak::readFile(const char *fileName, HeapAllocator &mem) {
    FILE *file = fopen(fileName, "rb");
    if (file == nullptr) {
        return {mem, nullptr, 0};
    }

    fseek(file, 0L, SEEK_END);
    u8 size = ftell(file);
    fseek(file, 0L, SEEK_SET);

    u1 *content = mem.alloc<u1>(size, __FILE__, __LINE__);
    fread(content, sizeof(u1), size, file);
    return {mem, content, size};
}