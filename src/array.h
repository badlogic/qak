#ifndef QAK_ARRAY_H
#define QAK_ARRAY_H

#include "memory.h"

namespace qak {
    template<typename T>
    class Array {
    public:
        Array(HeapAllocator &mem, u8 capacity = 0) : _mem(mem), _size(0), _capacity(0), _buffer(nullptr) {
            if (capacity > 0)
                ensureCapacity(capacity);
        }

        template<typename E>
        static void freeObjects(Array<E *> &items) {
            for (int i = (int) items.size() - 1; i >= 0; i--) {
                items._mem.freeObject(items[i], __FILE__, __LINE__);
                items.removeAt(i);
            }
        }

        ~Array() {
            clear();
            deallocate(_buffer);
        }

        inline void clear() {
            for (u8 i = 0; i < _size; ++i) {
                destroy(_buffer + (_size - 1 - i));
            }

            _size = 0;
        }

        inline void freeObjects() {
            Array::freeObjects(*this);
        }

        inline u8 capacity() const {
            return _capacity;
        }

        inline u8 size() const {
            return _size;
        }

        inline void setSize(u8 newSize, const T &defaultValue) {
            u8 oldSize = _size;
            _size = newSize;
            if (_capacity < newSize) {
                _capacity = (int) (_size * 1.75f);
                if (_capacity < 8) _capacity = 8;
                _buffer = _mem.realloc<T>(_buffer, _capacity, __FILE__, __LINE__);
            }
            if (oldSize < _size) {
                for (u8 i = oldSize; i < _size; i++) {
                    construct(_buffer + i, defaultValue);
                }
            }
        }

        inline void ensureCapacity(u8 newCapacity = 0) {
            if (_capacity >= newCapacity) return;
            _capacity = newCapacity;
            _buffer = _mem.realloc<T>(_buffer, newCapacity, __FILE__, __LINE__);
        }

        inline void add(const T &inValue) {
            if (_size == _capacity) {
                // inValue might reference an element in this buffer
                // When we reallocate, the reference becomes invalid.
                // We thus need to create a defensive copy before
                // reallocating.
                T valueCopy = inValue;
                _capacity = (int) (_size * 1.75f);
                if (_capacity < 8) _capacity = 8;
                _buffer = _mem.realloc<T>(_buffer, _capacity, __FILE__, __LINE__);
                construct(_buffer + _size++, valueCopy);
            } else {
                construct(_buffer + _size++, inValue);
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
            --_size;

            if (inIndex != _size) {
                for (u8 i = inIndex; i < _size; ++i) {
                    T tmp(_buffer[i]);
                    _buffer[i] = _buffer[i + 1];
                    _buffer[i + 1] = tmp;
                }
            }

            destroy(_buffer + _size);
        }

        inline bool contains(const T &inValue) {
            for (u8 i = 0; i < _size; ++i) {
                if (_buffer[i] == inValue) {
                    return true;
                }
            }

            return false;
        }

        inline int indexOf(const T &inValue) {
            for (u8 i = 0; i < _size; ++i) {
                if (_buffer[i] == inValue) {
                    return (int) i;
                }
            }

            return -1;
        }

        inline T &operator[](u8 inIndex) {
            return _buffer[inIndex];
        }

        inline friend bool operator==(Array<T> &lhs, Array<T> &rhs) {
            if (lhs._size() != rhs._size()) {
                return false;
            }

            for (u8 i = 0, n = lhs._size(); i < n; ++i) {
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
            return _buffer;
        }

    private:
        HeapAllocator &_mem;
        u8 _size;
        u8 _capacity;
        T *_buffer;

        inline T *allocate(u8 n) {
            T *ptr = _mem.calloc<T>(n, __FILE__, __LINE__);
            return ptr;
        }

        inline void deallocate(T *buffer) {
            if (buffer) {
                _mem.free(buffer, __FILE__, __LINE__);
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

    template <typename T>
    class FixedArray {
    public:
        FixedArray(BumpAllocator &mem, Array<T> &array): _mem(mem), _size(array.size()) {
            if (array.size() > 0) {
                _buffer = _mem.alloc<T>(_size);
                for (u8 i = 0; i < _size; i++) {
                    new(_buffer + i) T(array[i]);
                }
            }
        }

        inline T &operator[](u8 inIndex) {
            return _buffer[inIndex];
        }

        inline u8 size() const {
            return _size;
        }

    private:
        BumpAllocator &_mem;
        u8 _size;
        T *_buffer;
    };
}

#endif //QAK_ARRAY_H
