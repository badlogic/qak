#pragma once

#include "types.h"

typedef struct qak_allocation_header {
    size_t numBytes;
    const char* sourceFile;
    uint32_t line;
    struct qak_allocation_header* prev;
    struct qak_allocation_header* next;
} qak_allocation_header;

typedef struct qak_allocator {
    qak_allocation_header* head;

    void *(*allocate)(struct qak_allocator* self, size_t numBytes, const char* sourceFile, int line);

    void *(*reallocate)(struct qak_allocator* self, void *ptr, size_t numBytes, const char* sourceFile, int line);

    void (*free)(struct qak_allocator* self, void *ptr, const char* sourceFile, int line);
} qak_allocator;

void qak_allocator_init(qak_allocator* self);

void qak_allocator_print(qak_allocator* self);

size_t qak_allocator_num_allocated_bytes(qak_allocator* self);

size_t qak_allocator_num_allocations(qak_allocator* self);

#define QAK_ALLOCATE(allocator, type, numElements) ((allocator)->allocate((allocator), sizeof(type) * numElements, __FILE__, __LINE__))
#define QAK_REALLOCATE(allocator, ptr, type, numElements) ((allocator)->reallocate((allocator), ptr, sizeof(type) * numElements, __FILE__, __LINE__))
#define QAK_FREE(allocator, ptr) ((allocator)->free((allocator), ptr, __FILE__, __LINE__))