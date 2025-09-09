#include "mem_alloc_private.h"

void *mem_alloc(size_t size) {
	if (!size || size & (size - 1))
		return NULL;
	return NULL;
}

void mem_free(void *ptr) {
	if (!ptr) return;
}

void *mem_realloc(void *ptr, size_t new_size) {
	if (!ptr || !new_size) return NULL;
	return NULL;
}
