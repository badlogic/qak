#ifndef QAK_ARRAY_H
#define QAK_ARRAY_H

#include "memory.h"

namespace qak {

    template<typename T>
    class Array {
    private:
        HeapAllocator &_mem;
        size_t _size;
        size_t _capacity;
        T *_buffer;

        QAK_FORCE_INLINE T *allocate(size_t n) {
            T *ptr = _mem.calloc<T>(n, __FILE__, __LINE__);
            return ptr;
        }

        QAK_FORCE_INLINE void deallocate(T *buffer) {
            if (buffer) {
                _mem.free(buffer, __FILE__, __LINE__);
            }
        }

        QAK_FORCE_INLINE void construct(T *buffer, const T &val) {
            new(buffer) T(val);
        }

        QAK_FORCE_INLINE void destroy(T *buffer) {
            buffer->~T();
        }

    public:
        Array(HeapAllocator &mem, size_t capacity = 0) : _mem(mem), _size(0), _capacity(0), _buffer(nullptr) {
            if (capacity > 0)
                ensureCapacity(capacity);
        }

        ~Array() {
            clear();
            deallocate(_buffer);
        }

        QAK_FORCE_INLINE void clear() {
            for (size_t i = 0; i < _size; ++i) {
                destroy(_buffer + (_size - 1 - i));
            }

            _size = 0;
        }

        QAK_FORCE_INLINE void freeObjects() {
            for (int32_t i = (int32_t) size() - 1; i >= 0; i--) {
                _mem.freeObject(_buffer[i], __FILE__, __LINE__);
            }
            _size = 0;
        }

        QAK_FORCE_INLINE size_t capacity() const {
            return _capacity;
        }

        QAK_FORCE_INLINE size_t size() const {
            return _size;
        }

        QAK_FORCE_INLINE void setSize(size_t newSize, const T &defaultValue) {
            size_t oldSize = _size;
            _size = newSize;
            if (_capacity < newSize) {
                _capacity = (size_t) (_size * 1.75f);
                if (_capacity < 8) _capacity = 8;
                _buffer = _mem.realloc<T>(_buffer, _capacity, __FILE__, __LINE__);
            }
            if (oldSize < _size) {
                for (size_t i = oldSize; i < _size; i++) {
                    construct(_buffer + i, defaultValue);
                }
            }
        }

        QAK_FORCE_INLINE void ensureCapacity(size_t newCapacity = 0) {
            if (_capacity >= newCapacity) return;
            _capacity = newCapacity;
            _buffer = _mem.realloc<T>(_buffer, newCapacity, __FILE__, __LINE__);
        }

        QAK_FORCE_INLINE void add(const T &inValue) {
            if (_size == _capacity) {
                // inValue might reference an element in this buffer
                // When we reallocate, the reference becomes invalid.
                // We thus need to create a defensive copy before
                // reallocating.
                T valueCopy = inValue;
                _capacity = (size_t) (_size * 1.75f);
                if (_capacity < 8) _capacity = 8;
                _buffer = _mem.realloc<T>(_buffer, _capacity, __FILE__, __LINE__);
                construct(_buffer + _size++, valueCopy);
            } else {
                construct(_buffer + _size++, inValue);
            }
        }

        QAK_FORCE_INLINE void addAll(Array<T> &inValue) {
            ensureCapacity(this->size() + inValue.size());
            for (size_t i = 0; i < inValue.size(); i++) {
                add(inValue[i]);
            }
        }

        QAK_FORCE_INLINE void removeAt(size_t inIndex) {
            --_size;

            if (inIndex != _size) {
                for (size_t i = inIndex; i < _size; ++i) {
                    T tmp(_buffer[i]);
                    _buffer[i] = _buffer[i + 1];
                    _buffer[i + 1] = tmp;
                }
            }

            destroy(_buffer + _size);
        }

        QAK_FORCE_INLINE bool contains(const T &inValue) {
            for (size_t i = 0; i < _size; ++i) {
                if (_buffer[i] == inValue) {
                    return true;
                }
            }

            return false;
        }

        QAK_FORCE_INLINE int32_t indexOf(const T &inValue) {
            for (size_t i = 0; i < _size; ++i) {
                if (_buffer[i] == inValue) {
                    return (int32_t) i;
                }
            }

            return -1;
        }

        QAK_FORCE_INLINE T &operator[](size_t inIndex) {
            return _buffer[inIndex];
        }

        QAK_FORCE_INLINE T *buffer() {
            return _buffer;
        }
    };

    template<typename T>
    class FixedArray {
    private:
        BumpAllocator &_mem;
        size_t _size;
        T *_buffer;

    public:
        FixedArray(BumpAllocator &mem, Array<T> &array) : _mem(mem), _size(array.size()) {
            if (array.size() > 0) {
                _buffer = _mem.alloc<T>(_size);
                for (size_t i = 0; i < _size; i++) {
                    new(_buffer + i) T(array[i]);
                }
            }
        }

        QAK_FORCE_INLINE T &operator[](size_t inIndex) {
            return _buffer[inIndex];
        }

        QAK_FORCE_INLINE size_t size() const {
            return _size;
        }
    };
}

#endif //QAK_ARRAY_H
