#ifndef MEM_ALLOC_PRIVATE_H
#define MEM_ALLOC_PRIVATE_H

#include "mem_alloc.h"
#include <stdalign.h>
#include <string.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

#define ARENA_SIZE\
	1024LU * 127
#define MIN_ALLOC\
	alignof(max_align_t)
#define ROUNDUP(size, to)\
	(((size) + (to) - 1) & ~((to) - 1))
#define MEM_OFFSET\
	ROUNDUP(sizeof(ptr_t), MIN_ALLOC)
#define NUM_SIZE_CLASSES\
	(ARENA_SIZE - MEM_OFFSET) / MIN_ALLOC
#define SIZE_CLASS(size)\
	(size) / MIN_ALLOC
#define PTR(mem)\
	(ptr_t*)((unsigned char*)mem - MEM_OFFSET)

typedef struct arena arena_t;
typedef struct ptr ptr_t;

struct ptr {
	void *mem;
	size_t total_size;
	ptr_t *next;
	ptr_t *prev;
	ptr_t *next_free;
	ptr_t *prev_free;
	bool is_valid;
	bool is_mmap;
};

struct arena {
	alignas(max_align_t) unsigned char buff[ARENA_SIZE];
	ptr_t *free_ptr_tails[NUM_SIZE_CLASSES];
	ptr_t *ptrs_tail;
	size_t offset;
};

static inline void *use_free_list(size_t total_size, arena_t *arena) {
	ptr_t **free_tail =
		&arena->free_ptr_tails[SIZE_CLASS(total_size - MEM_OFFSET)];
	void *mem = (*free_tail)->mem;
	*free_tail = (*free_tail)->prev_free;
	return mem;
}

static inline void *use_mmap(size_t total_size) {
	ptr_t *ptr =
		(ptr_t*)mmap(
			NULL, total_size, PROT_WRITE | PROT_READ,
			MAP_ANONYMOUS | MAP_PRIVATE, -1, 0
		);
	if (!ptr)
		return NULL;
	ptr->mem = (void*)((unsigned char*)ptr + MEM_OFFSET);
	ptr->total_size = total_size;
	ptr->next = NULL;
	ptr->prev = NULL;
	ptr->next_free = NULL;
	ptr->prev_free = NULL;
	ptr->is_valid = true;
	ptr->is_mmap = true;

	return ptr->mem;
}

static inline void add_to_free_list(ptr_t *ptr, arena_t *arena) {
	size_t size = ptr->total_size - MEM_OFFSET;
	ptr_t **free_tail = 
		&arena->free_ptr_tails[SIZE_CLASS(size)];
	if (!*free_tail) {
		*free_tail = ptr;
	} else {
		ptr->prev_free = *free_tail;
		(*free_tail)->next_free = ptr;
		*free_tail = (*free_tail)->next_free;
	}
}

static inline void *use_arena(size_t total_size, arena_t *arena) {
	ptr_t *ptr = (ptr_t*)&arena->buff[arena->offset];
	ptr->mem = (void*)((unsigned char*)ptr + MEM_OFFSET);
	arena->offset += total_size;
	if (!arena->ptrs_tail) {
		ptr->prev = NULL;
		arena->ptrs_tail = ptr;
	} else {
		ptr->prev = arena->ptrs_tail;
		arena->ptrs_tail->next = ptr;
		arena->ptrs_tail = arena->ptrs_tail->next;
	}
	ptr->total_size = total_size;
	ptr->next = NULL;
	ptr->next_free = NULL;
	ptr->prev_free = NULL;
	ptr->is_valid = true;
	ptr->is_mmap = false;
	
	return ptr->mem;
}

#endif
