#include "memory_c.h"
#include "qak.h"

typedef struct _qak_block {
    uint8_t *base;
    uint8_t *end;
    uint8_t *nextFree;
    size_t size;
    struct _qak_block *next;
} _qak_block;

static QAK_FORCE_INLINE _qak_block* qak_block_new(size_t size) {
    _qak_block *block = QAK_ALLOC(_qak_block, 1);
    block->base = QAK_ALLOC(uint8_t, size);
    block->end = block->base + size;
    block->nextFree = block->base;
    block->next = NULL;
    return block;
}

static QAK_FORCE_INLINE void qak_block_delete(_qak_block* block) {
    QAK_FREE(block->base);
    QAK_FREE(block);
}

static QAK_FORCE_INLINE bool qak_block_can_store(_qak_block *block, size_t numBytes) {
    return block->nextFree + numBytes < block->end;
}

static QAK_FORCE_INLINE void *qak_block_alloc(_qak_block *block, size_t numBytes) {
    uint8_t *ptr = block->nextFree;
    block->nextFree += numBytes;
    return ptr;
}

typedef struct _qak_bump_allocator {
    _qak_block *first;
    uint32_t blockSize;
} _qak_block_allocator;

qak_block_allocator qak_block_allocator_new(size_t blockSize) {
    _qak_block_allocator *allocator = QAK_ALLOC(_qak_block_allocator, 1);
    allocator->first = NULL;
    allocator->blockSize = blockSize;
    return (qak_block_allocator)allocator;
}

void qak_block_allocator_delete(qak_block_allocator handle) {
    _qak_block_allocator *allocator = handle;
    _qak_block *block = allocator->first;
    while (block) {
        _qak_block *next = block->next;
        qak_block_delete(block);
        block = next;
    }
    QAK_FREE(allocator);
}

void* qak_block_allocator_allocate(qak_block_allocator handle, size_t numBytes) {
    if (numBytes == 0) return NULL;

    _qak_block_allocator *allocator = handle;
    if (allocator->first == NULL || !qak_block_can_store(allocator->first, numBytes)) {
        _qak_block *newFirst = qak_block_new(allocator->blockSize < numBytes ? numBytes * 2 : allocator->blockSize);
        newFirst->next = allocator->first;
        allocator->first = newFirst;
    }

    return qak_block_alloc(allocator->first, numBytes);
}