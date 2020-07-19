#include <stdio.h>
#include "memory.h"
#include "test.h"

using namespace qak;

int main() {
    Test test("Bump allocator");
    HeapAllocator mem;
    BumpAllocator allocator(mem, 16);

    uint8_t *fourBytes = allocator.alloc<uint8_t>(4);
    QAK_CHECK(fourBytes != nullptr, "Expected allocated memory.");
    QAK_CHECK(allocator.head->next == nullptr, "Expected single block.");
    QAK_CHECK(allocator.head->nextFree - allocator.head->base == 4, "Expected 4 allocated bytes.");

    uint8_t *eightBytes = allocator.alloc<uint8_t>(8);
    QAK_CHECK(eightBytes != nullptr, "Expected allocated memory.");
    QAK_CHECK(allocator.head->next == nullptr, "Expected single block.");
    QAK_CHECK(allocator.head->nextFree - allocator.head->base == 12, "Expected 12 allocated bytes.");

    uint8_t *moreBytes = allocator.alloc<uint8_t>(1000);
    QAK_CHECK(moreBytes != nullptr, "Expected allocated memory.");
    QAK_CHECK(allocator.head->next != nullptr, "Expected single block.");
    QAK_CHECK(allocator.head->nextFree - allocator.head->base == 1000, "Expected 1000 allocated bytes.");

    allocator.free();
    QAK_CHECK(allocator.head == nullptr, "Expected no block.");
}

