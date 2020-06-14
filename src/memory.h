#ifndef QAK_MEMORY_H
#define QAK_MEMORY_H

#include "types.h"
#include <map>

// Default block size of the BumpAllocator
#define QAK_BLOCK_SIZE (512 * 1024)

namespace qak {
    struct Allocation {
        void *address;
        u8 size;
        const char *fileName;
        int line;

        Allocation() : address(nullptr), size(0), fileName(nullptr), line(0) {}

        Allocation(void *address, u8 size, const char *file, int line) : address(address), size(size), fileName(file), line(line) {}
    };

    struct HeapAllocator {
    private:
        std::map<void *, Allocation> allocations;

    public:
        ~HeapAllocator() {
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
                printf("%s:%i (address %p): Double free or not allocated through qak::memory\n", file, line, (void *) ptr);
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

    struct Block {
        u1 *base;
        u1 *end;
        u1 *nextFree;
        u8 size;
        Block *next;

        Block(u8 size) : size(size), next(nullptr) {
            base = (u1 *) ::malloc(size);
            end = base + size;
            nextFree = base;
        }

        ~Block() {
            ::free(base);
        }

        bool canStore(u8 size) {
            return nextFree + size < end;
        }

        u1 *alloc(u8 size) {
            u1 *ptr = nextFree;
            nextFree += size;
            return ptr;
        }
    };

    struct BumpAllocator {
        Block *head;
        u8 blockSize;

        BumpAllocator() : head(nullptr), blockSize(QAK_BLOCK_SIZE) {
        }

        BumpAllocator(u8 blockSize) : head(nullptr), blockSize(blockSize) {
        };

        ~BumpAllocator() {
            while (head) {
                Block *block = head;
                head = block->next;
                delete block;
            }
        }

        template<typename T>
        T *alloc(u8 num) {
            u8 size = sizeof(T) * num;
            if (size == 0) return nullptr;

            if (head == nullptr || !head->canStore(size)) {
                Block *newHead = new Block(blockSize < size ? size * 2 : blockSize);
                newHead->next = head;
                head = newHead;
            }

            return (T *) head->alloc(size);
        }

        void free() {
            while (head) {
                Block *block = head;
                head = block->next;
                delete block;
            }
        }
    };

    struct Buffer {
        HeapAllocator &mem;
        u1 *data;
        u8 size;

        void free() {
            if (data != nullptr) mem.free(data, __FILE__, __LINE__);
        }
    };
}

#endif //QAK_MEMORY_H
