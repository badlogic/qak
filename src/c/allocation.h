#pragma once

#include "types.h"

typedef struct qak_heap_allocation_header {
    size_t numBytes;
    const char *sourceFile;
    uint32_t line;
    struct qak_heap_allocation_header *prev;
    struct qak_heap_allocation_header *next;
} qak_heap_allocation_header;

typedef struct qak_bump_block_header {
    uint8_t *base;
    size_t numBytes;
    size_t allocatedBytes;
    struct qak_bump_block_header *next;
} qak_bump_block_header;

typedef struct qak_bump_allocator_data {
    qak_bump_block_header *head;
    size_t blockSize;
} qak_bump_allocator_data;

typedef struct qak_allocator {
    void *data;

    void *(*allocate)(struct qak_allocator *self, size_t numBytes, const char *sourceFile, uint32_t line);

    void *(*reallocate)(struct qak_allocator *self, void *ptr, size_t numBytes, const char *sourceFile, uint32_t line);

    void (*free)(struct qak_allocator *self, void *ptr, const char *sourceFile, uint32_t line);

    void (*shutdown)(struct qak_allocator *self);

    size_t (*num_allocated_bytes)(struct qak_allocator *self);

    size_t (*num_allocations)(struct qak_allocator *self);

    void (*print)(struct qak_allocator *self);
} qak_allocator;

qak_allocator qak_heap_allocator_init();

qak_allocator qak_bump_allocator_init(size_t blockSize);

void qak_allocator_shutdown(qak_allocator *self);

size_t qak_allocator_num_allocated_bytes(qak_allocator *self);

size_t qak_allocator_num_allocations(qak_allocator *self);

void qak_allocator_print(qak_allocator *self);

#define QAK_ALLOCATE(allocator, type, numElements) ((allocator)->allocate((allocator), sizeof(type) * numElements, __FILE__, __LINE__))
#define QAK_REALLOCATE(allocator, ptr, type, numElements) ((allocator)->reallocate((allocator), ptr, sizeof(type) * numElements, __FILE__, __LINE__))
#define QAK_FREE(allocator, ptr) ((allocator)->free((allocator), ptr, __FILE__, __LINE__))

