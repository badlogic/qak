#ifndef QAK_MEMORY_H
#define QAK_MEMORY_H

#include <stdlib.h>

#define QAK_BLOCK_ALLOC(blockAllocator, type, numElements) qak_block_allocator_allocate(blockAllocator, sizeof(type) * numElements);
#define QAK_ALLOC(type, numElements) ((type*)malloc(sizeof(type) * numElements))
#define QAK_REALLOC(type, ptr, numElements) ((type*)realloc(ptr, sizeof(type) * numElements))
#define QAK_FREE(ptr) free(ptr)

typedef void *qak_block_allocator;

qak_block_allocator qak_block_allocator_new(size_t blockSize);

void qak_block_allocator_delete(qak_block_allocator handle);

void* qak_block_allocator_allocate(qak_block_allocator handle, size_t numBytes)

#endif