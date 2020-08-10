#include "c/allocation.h"
#include "test.h"
#include <string.h>

int main(int argc, char **argv) {
    QAK_UNUSED(argc);
    QAK_UNUSED(argv);

    qak_allocator mem;
    qak_allocator_init(&mem);

    uint8_t *alloc1 = QAK_ALLOCATE(&mem, uint8_t, 16);
    QAK_CHECK(alloc1, "Couldn't allocate array.");
    qak_allocation_header *header = (qak_allocation_header *) (alloc1 - sizeof(qak_allocation_header));
    QAK_CHECK(header->numBytes == 16, "Incorrect size in allocation header.");
    QAK_CHECK(!strcmp(header->sourceFile + strlen(header->sourceFile) - strlen("test_memory.c"), "test_memory.c"),
              "Incorrect source file in allocation header.");
    QAK_CHECK(header->line == 9, "Incorrect line in allocation header.");

    alloc1 = QAK_REALLOCATE(&mem, alloc1, uint8_t, 1);
    QAK_CHECK(alloc1, "Couldn't allocate array.");
    header = (qak_allocation_header *) (alloc1 - sizeof(qak_allocation_header));
    QAK_CHECK(header->numBytes == 1, "Incorrect size in allocation header.");
    QAK_CHECK(!strcmp(header->sourceFile + strlen(header->sourceFile) - strlen("test_memory.c"), "test_memory.c"),
              "Incorrect source file in allocation header.");
    QAK_CHECK(header->line == 16, "Incorrect line in allocation header.");

    uint8_t *alloc2 = QAK_ALLOCATE(&mem, uint8_t, 2);
    uint8_t *alloc3 = QAK_ALLOCATE(&mem, uint8_t, 3);
    uint8_t *alloc4 = QAK_ALLOCATE(&mem, uint8_t, 4);

    QAK_CHECK(qak_allocator_num_allocations(&mem) == 4, "Incorrect number of allocations.");
    QAK_CHECK(qak_allocator_num_allocated_bytes(&mem) == 2 + 3 + 4 + 1, "Incorrect number of allocated bytes.");

    QAK_FREE(&mem, alloc3);
    QAK_CHECK(qak_allocator_num_allocations(&mem) == 3, "Incorrect number of allocations.");
    QAK_CHECK(qak_allocator_num_allocated_bytes(&mem) == 2 + 4 + 1, "Incorrect number of allocated bytes.");

    alloc4 = QAK_REALLOCATE(&mem, alloc4, uint8_t, 9);
    QAK_CHECK(qak_allocator_num_allocations(&mem) == 3, "Incorrect number of allocations.");
    QAK_CHECK(qak_allocator_num_allocated_bytes(&mem) == 2 + 9 + 1, "Incorrect number of allocated bytes.");

    QAK_FREE(&mem, alloc1);
    QAK_CHECK(qak_allocator_num_allocations(&mem) == 2, "Incorrect number of allocations.");
    QAK_CHECK(qak_allocator_num_allocated_bytes(&mem) == 2 + 9, "Incorrect number of allocated bytes.");

    QAK_FREE(&mem, alloc2);
    QAK_CHECK(qak_allocator_num_allocations(&mem) == 1, "Incorrect number of allocations.");
    QAK_CHECK(qak_allocator_num_allocated_bytes(&mem) == 9, "Incorrect number of allocated bytes.");

    QAK_FREE(&mem, alloc4);
    QAK_CHECK(qak_allocator_num_allocations(&mem) == 0, "Incorrect number of allocations.");
    QAK_CHECK(qak_allocator_num_allocated_bytes(&mem) == 0, "Incorrect number of allocated bytes.");

    return 0;
}
