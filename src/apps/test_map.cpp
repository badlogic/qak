#include <stdio.h>
#include "qak.h"
#include "test.h"

using namespace qak;

struct IntHashFunction {
    uint64_t operator()(const int &key) const {
        return (uint64_t) key;
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
        QAK_CHECK(intMap.size() == 1, "Expected size of 1, got %zu.", intMap.size());

        MapEntry<int, int> *entry = intMap.get(1);
        QAK_CHECK(entry != nullptr, "Expected map entry for key 1.");
        QAK_CHECK(entry->key == 1, "Expected key 1, got %i", entry->key);
        QAK_CHECK(entry->value == 2, "Expected value 2, got %i", entry->value);

        IntIntMap::MapEntries entries = intMap.entries();
        QAK_CHECK(entries.hasNext(), "Expected an entry");
        entry = entries.next();
        QAK_CHECK(entry->key == 1, "Expected key 1, got %i", entry->key);
        QAK_CHECK(entry->value == 2, "Expected value 2, got %i", entry->value);
        QAK_CHECK(!entries.hasNext(), "Expected no more entries");

        intMap.remove(1);
        QAK_CHECK(intMap.size() == 0, "Expected map size 0, got %zu", intMap.size());
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
        QAK_CHECK(intMap.size() == 10, "Expected map size to be 10, got %zu", intMap.size());

        IntIntMap::MapEntries entries = intMap.entries();
        while (entries.hasNext()) {
            MapEntry<int, int> *entry = entries.next();
            sumKeys -= entry->key;
            sumValues -= entry->value;
        }
        QAK_CHECK(sumKeys == 0, "Expected sumKeys to be 0, got %i", sumKeys);
        QAK_CHECK(sumValues == 0, "Expected sumValues to be 0, got %i", sumValues);
    }
}

