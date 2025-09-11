#include "mem_alloc_private.h"

_Thread_local static arena_t g_arena;

#include <stdio.h>

#ifndef NDEBUG
_Thread_local static int g_is_arena_full;
static inline void warn_arena_full() {
	if (!g_is_arena_full) {
		g_is_arena_full = 1;
		printf("[MEM_ALLOC WARNING]:\n");
		printf("\tUsing mmap from now on.\n");
	}
}
#define WARN_ARENA_FULL\
	warn_arena_full()
#else
#define WARN_ARENA_FULL
#endif

arena_t *global_arena() {
	return &g_arena;
}

void reset_global_arena() {
	memset(&g_arena, 0, sizeof(arena_t));
}

void *mem_alloc(size_t size) {
	size_t total_size = MEM_OFFSET + ROUNDUP(size, MIN_ALLOC);
	ptr_t **free_tail = &g_arena.free_ptr_tails[SIZE_CLASS(size)];

	if (g_arena.offset + total_size > ARENA_SIZE) {
		WARN_ARENA_FULL;
		return use_mmap(total_size);
	} else if (*free_tail) {
		ptr_t *ptr = *free_tail;
		*free_tail = ptr->prev_free;
		return ptr->mem;
	} else {
		return use_arena(total_size, &g_arena);
	}
	return NULL;
}

int mem_free(void *ptr) {
	if (!ptr) return -1;
	if (!PTR(ptr)->is_valid) return -1;

	if (PTR(ptr)->is_mmap) {
		if (munmap(PTR(ptr), PTR(ptr)->total_size))
			return -1;
		return 0;
	} else if (!PTR(ptr)->next) {
		if (PTR(ptr)->prev)
			PTR(ptr)->prev->next = NULL;
		g_arena.offset -= PTR(ptr)->total_size;
		PTR(ptr)->is_valid = false;
		g_arena.ptrs_tail = g_arena.ptrs_tail->prev;
		return 1;
	} else {
		add_to_free_list(PTR(ptr), &g_arena);
		merge_free_ptrs(PTR(ptr), &g_arena);
		return 2;
	}
}
void *mem_realloc(void *ptr, size_t size) {
	if (!ptr) return NULL;
	if (!PTR(ptr)->is_valid) return NULL;

	size_t total_size = MEM_OFFSET + ROUNDUP(size, MIN_ALLOC);

	if (PTR(ptr)->total_size >= total_size) {
		return ptr;
	} else if (
		PTR(ptr)->next && !PTR(ptr)->next->is_valid &&
		PTR(ptr)->next->total_size + PTR(ptr)->total_size >= total_size
	) {
		if (mem_free(ptr) != 2) return NULL;
		PTR(ptr)->is_valid = true;
		return ptr;
	} else {
		size_t size_to_copy =
			PTR(ptr)->total_size > total_size ?
			total_size : PTR(ptr)->total_size;
		void *new_mem = mem_alloc(size);
		if (!new_mem) return NULL;
		memcpy(new_mem, PTR(ptr)->mem, size_to_copy);
		return new_mem;
	}
}
