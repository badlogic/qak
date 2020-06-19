#include <stdio.h>
#include "qak.h"
#include "test.h"

using namespace qak;

int main() {
    BumpAllocator allocator(16);

    u1 *fourBytes = allocator.alloc<u1>(4);
    QAK_CHECK(fourBytes != nullptr, "Expected allocated memory.");
    QAK_CHECK(allocator.head->next == nullptr, "Expected single block.");
    QAK_CHECK(allocator.head->nextFree - allocator.head->base == 4, "Expected 4 allocated bytes.");

    u1 *eightBytes = allocator.alloc<u1>(8);
    QAK_CHECK(eightBytes != nullptr, "Expected allocated memory.");
    QAK_CHECK(allocator.head->next == nullptr, "Expected single block.");
    QAK_CHECK(allocator.head->nextFree - allocator.head->base == 12, "Expected 12 allocated bytes.");

    u1 *moreBytes = allocator.alloc<u1>(1000);
    QAK_CHECK(moreBytes != nullptr, "Expected allocated memory.");
    QAK_CHECK(allocator.head->next != nullptr, "Expected single block.");
    QAK_CHECK(allocator.head->nextFree - allocator.head->base == 1000, "Expected 1000 allocated bytes.");

    allocator.free();
    QAK_CHECK(allocator.head == nullptr, "Expected no block.");
}

