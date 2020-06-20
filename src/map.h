#ifndef QAK_MAP_H
#define QAK_MAP_H

#include "memory.h"
#include "array.h"

namespace qak {
    template<typename K>
    struct HashFunction {
        u8 operator()(const K &key) const {
            return (u8) key;
        }
    };

    template<typename K>
    struct EqualsFunction {
        bool operator()(const K &a, const K &b) const {
            return (u8) a == (u8) b;
        }
    };

    template<typename K, typename V>
    struct MapEntry {
        K key;
        V value;
        MapEntry *next;

        MapEntry(K key, V value) : key(key), value(value), next(nullptr) {}
    };

    template<typename K, typename V, typename H = HashFunction<K>, typename E = EqualsFunction<K>>
    class Map {
        HeapAllocator &mem;
        Array<MapEntry<K, V> *> entries;
        u8 size;
        H hashFunc;
        E equalsFunc;

    public:
        struct MapEntries {
            Array<MapEntry<K, V> *> &entries;
            u8 index;
            MapEntry<K, V> *nextEntry;

            MapEntries(Array<MapEntry<K, V> *> &entries) : entries(entries), index(0), nextEntry(entries[0]) {}

            bool hasNext() {
                while (true) {
                    if (index >= entries.size()) return false;
                    if (nextEntry == nullptr) {
                        index++;
                        if (index < entries.size()) nextEntry = entries[index];
                        continue;
                    }
                    return true;
                }
            }

            MapEntry<K, V> *next() {
                MapEntry<K, V> *entry = nextEntry;
                if (entry != nullptr) nextEntry = entry->next;
                return entry;
            }
        };

        Map(HeapAllocator &mem) : mem(mem), entries(mem), size(0) {
            entries.setSize(16, nullptr);
        }

        Map(HeapAllocator &mem, u4 tableSize) : mem(mem), entries(mem), size(0) {
            entries.setSize(tableSize, nullptr);
        }

        ~Map() {
            for (u8 i = 0; i < entries.size(); i++) {
                MapEntry<K, V> *entry = entries[i];
                while (entry != nullptr) {
                    MapEntry<K, V> *next = entry->next;
                    mem.free(entry, __FILE__, __LINE__);
                    entry = next;
                }
            }
        }

        void put(const K &key, const V &value) {
            u8 hash = hashFunc(key) % entries.size();
            MapEntry<K, V> *entry = entries[hash];
            size++;

            // No entries for that hash, add a new entry
            if (entry == nullptr) {
                entry = new(mem.alloc<MapEntry<K, V>>(1, __FILE__, __LINE__)) MapEntry<K, V>(key, value);
                entries[hash] = entry;
                return;
            }

            while (entry != nullptr) {
                // Found key, replace key and value in entry.
                if (equalsFunc(key, entry->key)) {
                    entry->key = key;
                    entry->value = value;
                    return;
                }
                entry = entry->next;
            }

            // Didn't find key, add a new entry
            entry = new(mem.alloc<MapEntry<K, V>>(1, __FILE__, __LINE__)) MapEntry<K, V>(key, value);
            entry->next = entries[hash];
            entries[hash] = entry;
        }

        MapEntry<K, V> *get(const K &key) {
            u8 hash = hashFunc(key) % entries.size();
            MapEntry<K, V> *entry = entries[hash];
            while (entry != nullptr) {
                if (equalsFunc(key, entry->key)) break;
                entry = entry->next;
            }
            return entry;
        }

        void remove(const K &key) {
            u8 hash = hashFunc(key) % entries.size();
            MapEntry<K, V> *prevEntry = nullptr;
            MapEntry<K, V> *entry = entries[hash];
            while (entry != nullptr) {
                if (equalsFunc(key, entry->key)) {
                    if (prevEntry == nullptr) {
                        entries[hash] = entry->next;
                    } else {
                        prevEntry->next = entry->next;
                    }
                    entry->~MapEntry<K, V>();
                    mem.free(entry, __FILE__, __LINE__);
                    size--;
                    break;
                }
                prevEntry = entry;
                entry = entry->next;
            }
        }

        MapEntries getEntries() {
            return MapEntries(entries);
        }

        u8 getSize() {
            return size;
        }
    };
}

#endif //QAK_MAP_H
