#include "allocation.h"

#include <stdlib.h>
#include <stdio.h>

static void *heap_allocate(qak_allocator *self, size_t numBytes, const char *sourceFile, uint32_t line) {
    qak_heap_allocation_header *header = (qak_heap_allocation_header *) malloc(sizeof(qak_heap_allocation_header) + numBytes);
    header->numBytes = numBytes;
    header->sourceFile = sourceFile;
    header->line = line;
    header->prev = NULL;
    header->next = self->data;
    if (self->data) ((qak_heap_allocation_header *) self->data)->prev = header;
    self->data = header;
    return (uint8_t *) header + sizeof(qak_heap_allocation_header);
}

static void *heap_reallocate(qak_allocator *self, void *ptr, size_t numBytes, const char *sourceFile, uint32_t line) {
    qak_heap_allocation_header *header = (qak_heap_allocation_header *) ((uint8_t *) ptr - sizeof(qak_heap_allocation_header));
    qak_heap_allocation_header *originalHeader = header;
    header = realloc(header, sizeof(qak_heap_allocation_header) + numBytes);
    header->numBytes = numBytes;
    header->sourceFile = sourceFile;
    header->line = line;
    if (header->prev) header->prev->next = header;
    if (header->next) header->next->prev = header;
    if (self->data == originalHeader) self->data = header;
    return (uint8_t *) header + sizeof(qak_heap_allocation_header);;
}

static void heap_free(qak_allocator *self, void *ptr, const char *sourceFile, uint32_t line) {
    qak_heap_allocation_header *header = (qak_heap_allocation_header *) ((uint8_t *) ptr - sizeof(qak_heap_allocation_header));
    if (header->prev) header->prev->next = header->next;
    if (header->next) header->next->prev = header->prev;
    if (self->data == header) self->data = header->next;
    free(header);
}

static size_t heap_num_allocated_bytes(qak_allocator *self) {
    size_t numAllocatedBytes = 0;
    qak_heap_allocation_header *header = self->data;
    while (header) {
        numAllocatedBytes += header->numBytes;
        header = header->next;
    }
    return numAllocatedBytes;
}

static size_t heap_num_allocations(qak_allocator *self) {
    size_t numAllocations = 0;
    qak_heap_allocation_header *header = self->data;
    while (header) {
        numAllocations++;
        header = header->next;
    }
    return numAllocations;
}

static void heap_print(qak_allocator *self) {
    qak_heap_allocation_header *header = self->data;
    while (header) {
        printf("%p, %zu bytes %s:%i\n", (void *) header, header->numBytes, header->sourceFile, header->line);
        header = header->next;
    }
}

static void heap_shutdown(qak_allocator *self) {
    qak_heap_allocation_header *header = self->data;
    while (header) {
        qak_heap_allocation_header *curr = header;
        header = header->next;
        free(curr);
    }
    self->data = NULL;
}

static qak_bump_block_header *new_bump_block(size_t blockSize) {
    qak_bump_block_header *block = malloc(sizeof(qak_bump_block_header) + blockSize);
    block->base = ((uint8_t *) (block)) + sizeof(qak_bump_block_header);
    block->numBytes = blockSize;
    block->allocatedBytes = 0;
    block->next = NULL;
    return block;
}

static void *bump_allocate(qak_allocator *self, size_t numBytes, const char *sourceFile, uint32_t line) {
    qak_bump_allocator_data *data = ((qak_bump_allocator_data *) self->data);
    qak_bump_block_header *block = data->head;

    if (block == NULL) {
        block = data->head = new_bump_block(QAK_MAX(numBytes, data->blockSize));
    }

    if (block->numBytes - block->allocatedBytes < numBytes) {
        qak_bump_block_header *newBlock = new_bump_block(QAK_MAX(numBytes, data->blockSize));
        newBlock->next = block;
        block = data->head = newBlock;
    }

    uint8_t *ptr = block->base + block->allocatedBytes;
    block->allocatedBytes += numBytes;
    return ptr;
}

static void *bump_reallocate(qak_allocator *self, void *ptr, size_t numBytes, const char *sourceFile, uint32_t line) {
    fprintf(stderr, "Bump allocator does not support reallocate()\n");
    abort();
}

static void bump_free(qak_allocator *self, void *ptr, const char *sourceFile, uint32_t line) {
    fprintf(stderr, "Bump allocator does not support free()\n");
    abort();
}

static size_t bump_num_allocated_bytes(qak_allocator *self) {
    qak_bump_allocator_data *data = ((qak_bump_allocator_data *) self->data);
    qak_bump_block_header *block = data->head;
    size_t numAllocatedBytes = 0;
    while (block) {
        numAllocatedBytes += block->allocatedBytes;
        block = block->next;
    }
    return numAllocatedBytes;
}

static size_t bump_num_allocations(qak_allocator *self) {
    qak_bump_allocator_data *data = ((qak_bump_allocator_data *) self->data);
    qak_bump_block_header *block = data->head;
    size_t numAllocations = 0;
    while (block) {
        numAllocations++;
        block = block->next;
    }
    return numAllocations;
}

static void bump_print(qak_allocator *self) {
    qak_bump_allocator_data *data = ((qak_bump_allocator_data *) self->data);
    qak_bump_block_header *block = data->head;
    while (block) {
        printf("block %p, total: %zu, allocated: %zu\n", block->base, block->numBytes, block->allocatedBytes);
        block = block->next;
    }
}

static void bump_shutdown(qak_allocator *self) {
    qak_bump_allocator_data *data = ((qak_bump_allocator_data *) self->data);
    qak_bump_block_header *block = data->head;
    while (block) {
        qak_bump_block_header *next = block->next;
        free(block);
        block = next;
    }
    free(self->data);
    self->data = NULL;
}

qak_allocator qak_heap_allocator_init() {
    return (qak_allocator) {
            NULL,
            heap_allocate,
            heap_reallocate,
            heap_free,
            heap_shutdown,
            heap_num_allocated_bytes,
            heap_num_allocations,
            heap_print
    };
}

qak_allocator qak_bump_allocator_init(size_t blockSize) {
    qak_bump_allocator_data *data = malloc(sizeof(qak_bump_allocator_data));
    data->blockSize = blockSize;
    data->head = NULL;
    return (qak_allocator) {
            data,
            bump_allocate,
            bump_reallocate,
            bump_free,
            bump_shutdown,
            bump_num_allocated_bytes,
            bump_num_allocations,
            bump_print
    };
}

void qak_allocator_shutdown(qak_allocator *self) {
    self->shutdown(self);
}

size_t qak_allocator_num_allocated_bytes(qak_allocator *self) {
    return self->num_allocated_bytes(self);
}

size_t qak_allocator_num_allocations(qak_allocator *self) {
    return self->num_allocations(self);
}

void qak_allocator_print(qak_allocator *self) {
    self->print(self);
}
