#ifndef QAK_MAP_H
#define QAK_MAP_H

#include "memory.h"
#include "array.h"

namespace qak {

    template<typename K>
    struct HashFunction {
        int64_t operator()(const K &key) const {
            return (int64_t) key;
        }
    };

    template<typename K>
    struct EqualsFunction {
        bool operator()(const K &a, const K &b) const {
            return (int64_t) a == (int64_t) b;
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
    private:
        HeapAllocator &_mem;
        Array<MapEntry<K, V> *> _entries;
        size_t _size;
        H _hashFunc;
        E _equalsFunc;

    public:
        struct MapEntries {
            Array<MapEntry<K, V> *> &_entries;
            size_t _index;
            MapEntry<K, V> *_nextEntry;

            MapEntries(Array<MapEntry<K, V> *> &entries) : _entries(entries), _index(0), _nextEntry(entries[0]) {}

            bool hasNext() {
                while (true) {
                    if (_index >= _entries.size()) return false;
                    if (_nextEntry == nullptr) {
                        _index++;
                        if (_index < _entries.size()) _nextEntry = _entries[_index];
                        continue;
                    }
                    return true;
                }
            }

            MapEntry<K, V> *next() {
                MapEntry<K, V> *entry = _nextEntry;
                if (entry != nullptr) _nextEntry = entry->next;
                return entry;
            }
        };

        explicit Map(HeapAllocator &mem) : _mem(mem), _entries(mem), _size(0) {
            _entries.setSize(16, nullptr);
        }

        Map(HeapAllocator &mem, size_t tableSize) : _mem(mem), _entries(mem), _size(0) {
            _entries.setSize(tableSize, nullptr);
        }

        ~Map() {
            for (size_t i = 0; i < _entries.size(); i++) {
                MapEntry<K, V> *entry = _entries[i];
                while (entry != nullptr) {
                    MapEntry<K, V> *next = entry->next;
                    _mem.free(entry, QAK_SRC_LOC);
                    entry = next;
                }
            }
        }

        void put(const K &key, const V &value) {
            int64_t hash = _hashFunc(key) % _entries.size();
            MapEntry<K, V> *entry = _entries[hash];
            _size++;

            // No entries for that hash, add a new entry
            if (entry == nullptr) {
                entry = new(_mem.alloc<MapEntry<K, V>>(1, QAK_SRC_LOC)) MapEntry<K, V>(key, value);
                _entries[hash] = entry;
                return;
            }

            while (entry != nullptr) {
                // Found key, replace key and value in entry.
                if (_equalsFunc(key, entry->key)) {
                    entry->key = key;
                    entry->value = value;
                    return;
                }
                entry = entry->next;
            }

            // Didn't find key, add a new entry
            entry = new(_mem.alloc<MapEntry<K, V>>(1, QAK_SRC_LOC)) MapEntry<K, V>(key, value);
            entry->next = _entries[hash];
            _entries[hash] = entry;
        }

        MapEntry<K, V> *get(const K &key) {
            int64_t hash = _hashFunc(key) % _entries.size();
            MapEntry<K, V> *entry = _entries[hash];
            while (entry != nullptr) {
                if (_equalsFunc(key, entry->key)) break;
                entry = entry->next;
            }
            return entry;
        }

        void remove(const K &key) {
            int64_t hash = _hashFunc(key) % _entries.size();
            MapEntry<K, V> *prevEntry = nullptr;
            MapEntry<K, V> *entry = _entries[hash];
            while (entry != nullptr) {
                if (_equalsFunc(key, entry->key)) {
                    if (prevEntry == nullptr) {
                        _entries[hash] = entry->next;
                    } else {
                        prevEntry->next = entry->next;
                    }
                    entry->~MapEntry<K, V>();
                    _mem.free(entry, QAK_SRC_LOC);
                    _size--;
                    break;
                }
                prevEntry = entry;
                entry = entry->next;
            }
        }

        MapEntries entries() {
            return MapEntries(_entries);
        }

        size_t size() {
            return _size;
        }
    };
}

#endif //QAK_MAP_H
