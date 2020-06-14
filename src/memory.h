#ifndef QAK_MEMORY_H
#define QAK_MEMORY_H

#include "types.h"
#include <map>

namespace qak {
    struct Allocation {
        void *address;
        u8 size;
        const char *fileName;
        int line;

        Allocation() : address(nullptr), size(0), fileName(nullptr), line(0) {}

        Allocation(void *address, u8 size, const char *file, int line) : address(address), size(size), fileName(file), line(line) {}
    };

    struct MemoryArea {
    private:
        std::map<void *, Allocation> allocations;

    public:
        ~MemoryArea() {
            for (std::map<void *, Allocation>::iterator it = allocations.begin(); it != allocations.end(); it++) {
                ::free(it->second.address);
            }
        }

        template<typename T>
        T *alloc(u8 num, const char *file, u4 line) {
            u8 size = sizeof(T) * num;
            if (size == 0) return nullptr;

            T *ptr = (T *) ::malloc(size);
            allocations[(void *) ptr] = Allocation(ptr, size, file, line);
            return ptr;
        }

        template<typename T>
        T *calloc(u8 num, const char *file, u4 line) {
            u8 size = sizeof(T) * num;
            if (size == 0) return nullptr;

            T *ptr = (T *) ::malloc(size);
            ::memset(ptr, 0, size);
            allocations[(void *) ptr] = Allocation(ptr, size, file, line);
            return ptr;
        }

        template<typename T>
        T *realloc(T *ptr, u8 num, const char *file, u4 line) {
            u8 size = sizeof(T) * num;
            if (size == 0) return nullptr;

            T *result = nullptr;
            if (ptr == nullptr) {
                result = (T *) ::malloc(size);
            } else {
                result = (T *) ::realloc(ptr, size);
                allocations.erase(ptr);
            }
            allocations[(void *) result] = Allocation(ptr, size, file, line);
            return result;
        }

        template<typename T>
        void free(T *ptr, const char *file, u4 line) {
            if (allocations.count(ptr)) {
                ::free((void *) ptr);
                allocations.erase(ptr);
            } else {
                printf("%s:%i (address %p): Double free or not allocated through qak::memory\n", file, line, ptr);
            }
        }

        void printAllocations() {
            if (allocations.size() > 0) {
                for (std::map<void *, Allocation>::iterator it = allocations.begin(); it != allocations.end(); it++) {
                    printf("%s:%i (%llu bytes at %p)\n", it->second.fileName, it->second.line, it->second.size, it->second.address);
                }
            } else {
                printf("No allocations.");
            }
        }
    };

    struct Buffer {
        MemoryArea &mem;
        u1 *data;
        u8 size;

        void free() {
            if (data != nullptr) mem.free(data, __FILE__, __LINE__);
        }
    };
}

#endif //QAK_MEMORY_H
