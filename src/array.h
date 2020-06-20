#ifndef QAK_ARRAY_H
#define QAK_ARRAY_H

#include "memory.h"

namespace qak {
    template<typename T>
    class Array {
    public:
        Array(HeapAllocator &mem, u8 capacity = 0) : mem(mem), size(0), capacity(0), buffer(nullptr) {
            if (capacity > 0)
                ensureCapacity(capacity);
        }

        ~Array() {
            clear();
            deallocate(buffer);
        }

        inline void clear() {
            for (u8 i = 0; i < size; ++i) {
                destroy(buffer + (size - 1 - i));
            }

            size = 0;
        }

        inline u8 getCapacity() const {
            return capacity;
        }

        inline u8 getSize() const {
            return size;
        }

        inline void setSize(u8 newSize, const T &defaultValue) {
            u8 oldSize = size;
            size = newSize;
            if (capacity < newSize) {
                capacity = (int) (size * 1.75f);
                if (capacity < 8) capacity = 8;
                buffer = mem.realloc<T>(buffer, capacity, __FILE__, __LINE__);
            }
            if (oldSize < size) {
                for (u8 i = oldSize; i < size; i++) {
                    construct(buffer + i, defaultValue);
                }
            }
        }

        inline void ensureCapacity(u8 newCapacity = 0) {
            if (capacity >= newCapacity) return;
            capacity = newCapacity;
            buffer = mem.realloc<T>(buffer, newCapacity, __FILE__, __LINE__);
        }

        inline void add(const T &inValue) {
            if (size == capacity) {
                // inValue might reference an element in this buffer
                // When we reallocate, the reference becomes invalid.
                // We thus need to create a defensive copy before
                // reallocating.
                T valueCopy = inValue;
                capacity = (int) (size * 1.75f);
                if (capacity < 8) capacity = 8;
                buffer = mem.realloc<T>(buffer, capacity, __FILE__, __LINE__);
                construct(buffer + size++, valueCopy);
            } else {
                construct(buffer + size++, inValue);
            }
        }

        inline void addAll(Array<T> &inValue) {
            ensureCapacity(this->size() + inValue.size());
            for (u8 i = 0; i < inValue.size(); i++) {
                add(inValue[i]);
            }
        }

        inline void clearAndAddAll(Array<T> &inValue) {
            this->clear();
            this->addAll(inValue);
        }

        inline void removeAt(u8 inIndex) {
            assert(inIndex < size);

            --size;

            if (inIndex != size) {
                for (u8 i = inIndex; i < size; ++i) {
                    T tmp(buffer[i]);
                    buffer[i] = buffer[i + 1];
                    buffer[i + 1] = tmp;
                }
            }

            destroy(buffer + size);
        }

        inline bool contains(const T &inValue) {
            for (u8 i = 0; i < size; ++i) {
                if (buffer[i] == inValue) {
                    return true;
                }
            }

            return false;
        }

        inline int indexOf(const T &inValue) {
            for (u8 i = 0; i < size; ++i) {
                if (buffer[i] == inValue) {
                    return (int) i;
                }
            }

            return -1;
        }

        inline T &operator[](u8 inIndex) {
            assert(inIndex < size);

            return buffer[inIndex];
        }

        inline friend bool operator==(Array<T> &lhs, Array<T> &rhs) {
            if (lhs.size() != rhs.size()) {
                return false;
            }

            for (u8 i = 0, n = lhs.size(); i < n; ++i) {
                if (lhs[i] != rhs[i]) {
                    return false;
                }
            }

            return true;
        }

        inline friend bool operator!=(Array<T> &lhs, Array<T> &rhs) {
            return !(lhs == rhs);
        }

        inline T *getBuffer() {
            return buffer;
        }

    private:
        HeapAllocator &mem;
        u8 size;
        u8 capacity;
        T *buffer;

        inline T *allocate(u8 n) {
            T *ptr = mem.calloc<T>(n, __FILE__, __LINE__);

            assert(ptr);

            return ptr;
        }

        inline void deallocate(T *buffer) {
            if (buffer) {
                mem.free(buffer, __FILE__, __LINE__);
            }
        }

        inline void construct(T *buffer, const T &val) {
            new(buffer) T(val);
        }

        inline void destroy(T *buffer) {
            buffer->~T();
        }

        // Array &operator=(const Array &inArray) {};
    };
}

#endif //QAK_ARRAY_H
