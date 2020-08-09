#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "allocation.h"

#ifdef _MSC_VER
#  pragma warning(disable : 4127)      /* disable: C4127: conditional expression is constant */
#  define QAK_FORCE_INLINE __forceinline
#else
#  if defined (__cplusplus) || defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L   /* C99 */
#    ifdef __GNUC__
#      define QAK_FORCE_INLINE inline __attribute__((always_inline))
#    else
#      define QAK_FORCE_INLINE inline
#    endif
#  else
#    define QAK_FORCE_INLINE
#  endif /* __STDC_VERSION__ */
#endif

#define QAK_STR(str) str, sizeof(str) - 1
#define QAK_SRC_LOC __FILE__, __LINE__
#define QAK_MAX(a, b) (a > b ? a : b)

#define QAK_ARRAY_DECLARE(name, itemType) \
    typedef struct name { qak_allocator *allocator; int size; int capacity; itemType* items; } name; \
    name* name##_new(qak_allocator *allocator, int initialCapacity); \
    void name##_delete(name* self); \
    void name##_clear(name* self); \
    name* name##_set_size(name* self, int newSize); \
    void name##_ensure_capacity(name* self, int newCapacity); \
    void name##_add(name* self, itemType value); \
    void name##_add_all(name* self, name* other); \
    void name##_add_all_values(name* self, itemType* values, int offset, int count); \
    void name##_remove_at(name* self, int index); \
    int name##_contains(name* self, itemType value); \
    itemType name##_pop(name* self); \
    itemType name##_peek(name* self);

#define QAK_ARRAY_IMPLEMENT(name, itemType) \
    name* name##_new(qak_allocator *allocator, int initialCapacity) { \
        name* array = allocator->allocate(allocator, sizeof(name), __FILE__, __LINE__); \
        array->size = 0; \
        array->capacity = initialCapacity; \
        array->items = (itemType*)allocator->allocate(allocator, sizeof(itemType) * initialCapacity, __FILE__, __LINE__); \
        array->allocator = allocator; \
        return array; \
    } \
    void name##_delete(name* self) { \
        self->allocator->free(self->allocator, self->items, __FILE__, __LINE__); \
        self->allocator->free(self->allocator, self, __FILE__, __LINE__); \
    } \
    void name##_clear(name* self) { \
        self->size = 0; \
    } \
    name* name##_set_size(name* self, int newSize) { \
        self->size = newSize; \
        if (self->capacity < newSize) { \
            self->capacity = QAK_MAX(8, (int)(self->size * 1.75f)); \
            self->items = (itemType*)self->allocator->reallocate(self->allocator, self->items, sizeof(itemType) * self->capacity, __FILE__, __LINE__); \
        } \
        return self; \
    } \
    void name##_ensure_capacity(name* self, int newCapacity) { \
        if (self->capacity >= newCapacity) return; \
        self->capacity = newCapacity; \
        self->items = (itemType*)self->allocator->reallocate(self->allocator, self->items, sizeof(itemType) * self->capacity, __FILE__, __LINE__); \
    } \
    void name##_add(name* self, itemType value) { \
        if (self->size == self->capacity) { \
            self->capacity = QAK_MAX(8, (int)(self->size * 1.75f)); \
            self->items = (itemType*)self->allocator->reallocate(self->allocator, self->items, sizeof(itemType) * self->capacity, __FILE__, __LINE__); \
        } \
        self->items[self->size++] = value; \
    } \
    void name##_add_all(name* self, name* other) { \
        int i = 0; \
        for (; i < other->size; i++) { \
            name##_add(self, other->items[i]); \
        } \
    } \
    void name##_add_all_values(name* self, itemType* values, int offset, int count) { \
        int i = offset, n = offset + count; \
        for (; i < n; i++) { \
            name##_add(self, values[i]); \
        } \
    } \
    void name##_remove_at(name* self, int index) { \
        self->size--; \
        memmove(self->items + index, self->items + index + 1, sizeof(itemType) * (self->size - index)); \
    } \
    itemType name##_pop(name* self) { \
        itemType item = self->items[--self->size]; \
        return item; \
    } \
    itemType name##_peek(name* self) { \
        return self->items[self->size - 1]; \
    }
