#ifndef QAK_ARRAY_H
#define QAK_ARRAY_H

#ifdef __cplusplus
extern "C" {
#endif

#define QAK_ARRAY_DECLARE(name, itemType) \
	typedef struct name { int size; int capacity; itemType* items; } name; \
	name* name##_new(int initialCapacity); \
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
	name* name##_new(int initialCapacity) { \
		name* array = QAK_ALLOC(name, 1); \
		array->size = 0; \
		array->capacity = initialCapacity; \
		array->items = QAK_ALLOC(itemType, initialCapacity); \
		return array; \
	} \
	void name##_delete(name* self) { \
		QAK_FREE(self->items); \
		QAK_FREE(self); \
	} \
	void name##_clear(name* self) { \
		self->size = 0; \
	} \
	name* name##_set_size(name* self, int newSize) { \
		self->size = newSize; \
		if (self->capacity < newSize) { \
			self->capacity = MAX(8, (int)(self->size * 1.75f)); \
			self->items = QAK_REALLOC(self->items, itemType, self->capacity); \
		} \
		return self; \
	} \
	void name##_ensure_capacity(name* self, int newCapacity) { \
		if (self->capacity >= newCapacity) return; \
		self->capacity = newCapacity; \
		self->items = QAK_REALLOC(self->items, itemType, self->capacity); \
	} \
	void name##_add(name* self, itemType value) { \
		if (self->size == self->capacity) { \
			self->capacity = MAX(8, (int)(self->size * 1.75f)); \
			self->items = QAK_REALLOC(self->items, itemType, self->capacity); \
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
	int name##_contains(name* self, itemType value) { \
		itemType* items = self->items; \
		int i, n; \
		for (i = 0, n = self->size; i < n; i++) { \
			if (items[i] == value) return -1; \
		} \
		return 0; \
	} \
	itemType name##_pop(name* self) { \
		itemType item = self->items[--self->size]; \
		return item; \
	} \
	itemType name##_peek(name* self) { \
		return self->items[self->size - 1]; \
	}

#ifdef __cplusplus
}
#endif

#endif
