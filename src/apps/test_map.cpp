#include <stdio.h>
#include "qak.h"
#include "test.h"

using namespace qak;

struct IntHashFunction {
    u8 operator()(const int &key) const {
        return (u8) key;
    }
};

struct IntEqualsFunction {
    bool operator()(const int &a, const int &b) const {
        return a == b;
    }
};

typedef Map<int, int, IntHashFunction, IntEqualsFunction> IntIntMap;

int main() {
    Test test("Map");
    HeapAllocator mem;
    {
        IntIntMap intMap(mem);

        intMap.put(1, 2);
        QAK_CHECK(intMap.getSize() == 1, "Expected size of 1, got %llu.", intMap.getSize());

        MapEntry<int, int> *entry = intMap.get(1);
        QAK_CHECK(entry != nullptr, "Expected map entry for key 1.");
        QAK_CHECK(entry->key == 1, "Expected key 1, got %i", entry->key);
        QAK_CHECK(entry->value == 2, "Expected value 2, got %i", entry->value);

        IntIntMap::MapEntries entries = intMap.getEntries();
        QAK_CHECK(entries.hasNext(), "Expected an entry");
        entry = entries.next();
        QAK_CHECK(entry->key == 1, "Expected key 1, got %i", entry->key);
        QAK_CHECK(entry->value == 2, "Expected value 2, got %i", entry->value);
        QAK_CHECK(!entries.hasNext(), "Expected no more entries");

        intMap.remove(1);
        QAK_CHECK(intMap.getSize() == 0, "Expected map size 0, got %llu", intMap.getSize());
        QAK_CHECK(intMap.get(1) == nullptr, "Expected no entry for key 1");
    }
    mem.printAllocations();
    {
        IntIntMap intMap(mem);
        int sumKeys = 0;
        int sumValues = 0;
        for (int i = 0; i < 10; i++) {
            intMap.put(i, i * 10);
            sumKeys += i;
            sumValues += i * 10;
        }
        QAK_CHECK(intMap.getSize() == 10, "Expected map size to be 10, got %llu", intMap.getSize());

        IntIntMap::MapEntries entries = intMap.getEntries();
        while (entries.hasNext()) {
            MapEntry<int, int> *entry = entries.next();
            sumKeys -= entry->key;
            sumValues -= entry->value;
        }
        QAK_CHECK(sumKeys == 0, "Expected sumKeys to be 0, got %i", sumKeys);
        QAK_CHECK(sumValues == 0, "Expected sumValues to be 0, got %i", sumValues);
    }
}

