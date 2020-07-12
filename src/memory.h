#ifndef QAK_MEMORY_H
#define QAK_MEMORY_H

#include "types.h"
#include <map>
#include <string.h>
#include <cstdio>

// Default block size of the BumpAllocator
#define QAK_BLOCK_SIZE (512 * 16)

namespace qak {

    struct Allocation {
        void *address;
        size_t size;
        const char *fileName;
        int32_t line;

        Allocation() : address(nullptr), size(0), fileName(nullptr), line(0) {}

        Allocation(void *address, size_t size, const char *file, int32_t line) : address(address), size(size), fileName(file), line(line) {}
    };

    class HeapAllocator {
    private:
        std::map<void *, Allocation> _allocations;
        size_t _totalAllocations;
        size_t _totalFrees;

    public:
        HeapAllocator() : _totalAllocations(0), _totalFrees(0) {};

        HeapAllocator(const HeapAllocator &other) = delete;

        ~HeapAllocator() {
            for (std::map<void *, Allocation>::iterator it = _allocations.begin(); it != _allocations.end(); it++) {
                ::free(it->second.address);
            }
        }

        template<typename T>
        T *alloc(size_t num, const char *file, int32_t line) {
            size_t size = sizeof(T) * num;
            if (size == 0) return nullptr;

            _totalAllocations++;

            T *ptr = (T *) ::malloc(size);
            _allocations[(void *) ptr] = Allocation(ptr, size, file, line);
            return ptr;
        }

        template<typename T, typename ... ARGS>
        T *allocObject(const char *file, int32_t line, ARGS &&...args) {
            T *obj = new(alloc<T>(1, file, line)) T(std::forward<ARGS>(args)...);
            return obj;
        }

        template<typename T>
        T *calloc(size_t num, const char *file, int32_t line) {
            size_t size = sizeof(T) * num;
            if (size == 0) return nullptr;

            _totalAllocations++;

            T *ptr = (T *) ::malloc(size);
            ::memset(ptr, 0, size);
            _allocations[(void *) ptr] = Allocation(ptr, size, file, line);
            return ptr;
        }

        template<typename T>
        T *realloc(T *ptr, size_t num, const char *file, int32_t line) {
            size_t size = sizeof(T) * num;
            if (size == 0) return nullptr;

            _totalAllocations++;

            T *result = nullptr;
            if (ptr == nullptr) {
                result = (T *) ::malloc(size);
            } else {
                result = (T *) ::realloc(ptr, size);
                _allocations.erase(ptr);
            }
            _allocations[(void *) result] = Allocation(result, size, file, line);
            return result;
        }

        template<typename E>
        void freeObject(E *ptr, const char *file, int32_t line) {
            ptr->~E();
            free(ptr, file, line);
        }

        void free(void *ptr, const char *file, int32_t line) {
            if (_allocations.count(ptr)) {
                ::free((void *) ptr);
                _allocations.erase(ptr);
                _totalFrees++;
            } else {
                printf("%s:%i (address %p): Double free or not allocated through qak::memory\n", file, line, (void *) ptr);
            }
        }

        void printAllocations() {
            if (_allocations.size() > 0) {
                uint64_t totalSize = 0;
                for (std::map<void *, Allocation>::iterator it = _allocations.begin(); it != _allocations.end(); it++) {
                    printf("%s:%i (%zu bytes at %p)\n", it->second.fileName, it->second.line, it->second.size, it->second.address);
                    totalSize += it->second.size;
                }
                printf("Total memory: %llu, #allocations: %zu\n", (unsigned long long) totalSize, _allocations.size());
            } else {
                printf("No allocations.");
            }
        }

        size_t numAllocations() {
            return _allocations.size();
        }

        size_t totalAllocations() {
            return _totalAllocations;
        }

        size_t totalFrees() {
            return _totalFrees;
        }
    };

    struct Block {
        uint8_t *base;
        uint8_t *end;
        uint8_t *nextFree;
        size_t size;
        Block *next;

        Block(size_t size) : size(size), next(nullptr) {
            base = (uint8_t *) ::malloc(size);
            end = base + size;
            nextFree = base;
        }

        ~Block() {
            ::free(base);
        }

        QAK_FORCE_INLINE bool canStore(size_t size) {
            return nextFree + size < end;
        }

        QAK_FORCE_INLINE uint8_t *alloc(size_t size) {
            uint8_t *ptr = nextFree;
            nextFree += size;
            return ptr;
        }
    };

    struct BumpAllocator {
        Block *head;
        size_t blockSize;

        BumpAllocator() : head(nullptr), blockSize(QAK_BLOCK_SIZE) {
        }

        BumpAllocator(size_t blockSize) : head(nullptr), blockSize(blockSize) {
        };

        BumpAllocator(BumpAllocator const &) = delete;

        ~BumpAllocator() {
            free();
        }

        template<typename T, typename ... ARGS>
        T *allocObject(ARGS &&...args) {
            T *obj = new(alloc<T>(1)) T(std::forward<ARGS>(args)...);
            return obj;
        }

        template<typename T>
        T *alloc(size_t num) {
            size_t size = sizeof(T) * num;
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
}

#endif //QAK_MEMORY_H
