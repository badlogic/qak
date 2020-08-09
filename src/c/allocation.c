#include "allocation.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static void *qak_allocate(qak_allocator *self, size_t numBytes, const char *sourceFile, uint32_t line) {
    qak_allocation_header *header = (qak_allocation_header *) malloc(sizeof(qak_allocation_header) + numBytes);
    header->numBytes = numBytes;
    header->sourceFile = sourceFile;
    header->line = line;
    header->prev = NULL;
    header->next = self->head;
    if (self->head) self->head->prev = header;
    self->head = header;
    return (uint8_t *) header + sizeof(qak_allocation_header);
}

static void *qak_reallocate(qak_allocator *self, void *ptr, size_t numBytes, const char *sourceFile, uint32_t line) {
    qak_allocation_header *header = (qak_allocation_header *) ((uint8_t *) ptr - sizeof(qak_allocation_header));
    qak_allocation_header *originalHeader = header;
    header = realloc(header, sizeof(qak_allocation_header) + numBytes);
    header->numBytes = numBytes;
    header->sourceFile = sourceFile;
    header->line = line;
    if (header->prev) header->prev->next = header;
    if (header->next) header->next->prev = header;
    if (self->head == originalHeader) self->head = header;
    return (uint8_t *) header + sizeof(qak_allocation_header);;
}

static void qak_free(qak_allocator *self, void *ptr, const char *sourceFile, uint32_t line) {
    qak_allocation_header *header = (qak_allocation_header *) ((uint8_t *) ptr - sizeof(qak_allocation_header));
    if (header->prev) header->prev->next = header->next;
    if (header->next) header->next->prev = header->prev;
    if (self->head == header) self->head = header->next;
    free(header);
}

void qak_allocator_init(qak_allocator *self) {
    self->allocate = qak_allocate;
    self->reallocate = qak_reallocate;
    self->free = qak_free;
    self->head = NULL;
}

void qak_allocator_shutdown(qak_allocator *self) {
    qak_allocation_header *header = self->head;
    while (header) {
        qak_allocation_header *curr = header;
        header = header->next;
        free(curr);
    }
    self->head = NULL;
}

size_t qak_allocator_num_allocated_bytes(qak_allocator *self) {
    size_t numAllocatedBytes = 0;
    qak_allocation_header *header = self->head;
    while (header) {
        numAllocatedBytes += header->numBytes;
        header = header->next;
    }
    return numAllocatedBytes;
}

size_t qak_allocator_num_allocations(qak_allocator *self) {
    size_t numAllocations = 0;
    qak_allocation_header *header = self->head;
    while (header) {
        numAllocations++;
        header = header->next;
    }
    return numAllocations;
}

void qak_allocator_print(qak_allocator *self) {
    qak_allocation_header *header = self->head;
    while (header) {
        printf("%p, %zu bytes %s:%i\n", (void *) header, header->numBytes, header->sourceFile, header->line);
        header = header->next;
    }
}
