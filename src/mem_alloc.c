#include "mem_alloc_private.h"

_Thread_local static arena_t g_arena;

void *mem_alloc(size_t size) {
	if (!size || size & (size - 1))
		return NULL;

	size_t total_size = MEM_OFFSET + ROUNDUP(size, MIN_ALLOC);

	if (g_arena.free_ptr_tails[SIZE_CLASS(size)]) {
		return use_free_list(total_size, &g_arena);
	} else if (g_arena.offset + total_size > ARENA_SIZE) {
		return use_mmap(total_size);
	} else {
		return use_arena(total_size, &g_arena);
	}
}

void mem_free(void *ptr) {
	if (!ptr) return;
}

void *mem_realloc(void *ptr, size_t new_size) {
	if (!ptr || !new_size) return NULL;
	return NULL;
}
