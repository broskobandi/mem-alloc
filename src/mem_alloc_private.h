/*
MIT License

Copyright (c) 2025 broskobandi

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/** \file src/mem_alloc_private.h
 * \brief Private header file for the mem_alloc library. 
 * \details This file contains the definitions of private funcions and 
 * macros for the mem_alloc library. */

#ifndef MEM_ALLOC_PRIVATE_H
#define MEM_ALLOC_PRIVATE_H

#include "mem_alloc.h"
#include <stdalign.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>

/******************************************************************************
 * Macro definitions
 *****************************************************************************/

#define ARENA_SIZE_DEFAULT 1024LU * 128
#ifndef ARENA_SIZE_MULTIPLIER
#define ARENA_SIZE\
	ARENA_SIZE_DEFAULT
#else
#define ARENA_SIZE ARENA_SIZE_DEFAULT * ARENA_SIZE_MULTIPLIER
#endif
#define ROUNDUP(size, to)\
	(((size) + (to) - 1) & ~((to) - 1))
#define MIN_ALLOC\
	alignof(max_align_t)
#define MEM_OFFSET\
	ROUNDUP(sizeof(ptr_t), MIN_ALLOC)
#define PTR(mem)\
	((ptr_t*)((unsigned char*)(mem) - MEM_OFFSET))
#define NUM_SIZE_CLASSES\
	(ARENA_SIZE - MEM_OFFSET) / MIN_ALLOC
#define SIZE_CLASS(size)\
	(size) / MIN_ALLOC

/******************************************************************************
 * Struct definitions
 *****************************************************************************/

typedef struct ptr ptr_t;
typedef struct arena arena_t;

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

/******************************************************************************
 * Forward declarations of the helper functions for the test utility.
 *****************************************************************************/

arena_t *global_arena();
void reset_global_arena();

/******************************************************************************
 * Helper functions used by the public functions.
 *****************************************************************************/

/** Allocates memory in the heap. This function acts as a wrapper 
 * areound mmap().
 * \param total_size The total size, including the size of metadata
 * and padding, to be allocated.
 * \return A pointer to the allocated memory or NULL on failure. */
static inline void *use_mmap(size_t total_size) {
	ptr_t *ptr = (ptr_t*)mmap(
		NULL,
		MEM_OFFSET + total_size,
		PROT_WRITE | PROT_READ,
		MAP_ANONYMOUS | MAP_PRIVATE,
		-1, 0
	);
	if (!ptr) return NULL;

	ptr->is_mmap = true;
	ptr->is_valid = true;
	ptr->next = NULL;
	ptr->prev = NULL;
	ptr->next_free = NULL;
	ptr->prev_free = NULL;
	ptr->mem = (void*)((unsigned char*)ptr + MEM_OFFSET);
	ptr->total_size = ROUNDUP(total_size, (size_t)getpagesize());

	return ptr->mem;
}

/** Allocates memory in the arena.
 * This functions assumes that all arguments
 * passed to it were validated by the caller.
 * \param total_size The total size, including the size of metadata 
 * and padding, to be allocated.
 * \param arena A pointer to the arena to be used for the allocation.
 * \return A pointer to the allocated memory or NULL on failure. */
static inline void *use_arena(size_t total_size, arena_t *arena) {
	ptr_t *ptr = (ptr_t*)&arena->buff[arena->offset];
	arena->offset += total_size;
	ptr->mem = (void*)((unsigned char*)ptr + MEM_OFFSET);
	ptr->total_size = total_size;
	ptr->is_valid = true;
	ptr->is_mmap = false;
	ptr->next = NULL;
	ptr_t **tail = &arena->ptrs_tail;
	if (*tail) {
		ptr->prev = *tail;
		(*tail)->next = ptr;
		(*tail) = ptr;
	} else {
		*tail = ptr;
		ptr->prev = NULL;
	}
	ptr->next_free = NULL;
	ptr->prev_free = NULL;
	return ptr->mem;
}

/** Adds pointer metadata to the free list.
 * This functions assumes that all arguments
 * passed to it were validated by the caller.
 * \param ptr A pointer to the metadata to be added to the free list.
 * \param arena A pointer to the arena in use. */
static inline void add_to_free_list(ptr_t *ptr, arena_t *arena) {
	ptr->is_valid = false;
	ptr_t **free_tail =
		&arena->free_ptr_tails[SIZE_CLASS(ptr->total_size - MEM_OFFSET)];
	if (*free_tail) {
		ptr->prev_free = *free_tail;
		(*free_tail)->next_free = ptr;
		*free_tail = ptr;
	} else {
		*free_tail = ptr;
	}
}

/** Removes pointer metadata from the free list.
 * This functions assumes that all arguments
 * passed to it were validated by the caller.
 * \param ptr A pointer to the metadata to be removed from the free list.
 * \param arena A pointer to the arena in use. */
static inline void remove_from_free_list(ptr_t *ptr, arena_t *arena) {
	ptr_t **free_tail = 
		&arena->free_ptr_tails[SIZE_CLASS(ptr->total_size - MEM_OFFSET)];
	if (*free_tail == ptr)
		*free_tail = (*free_tail)->prev_free;
	if (ptr->next_free)
		ptr->next_free->prev_free = ptr->prev_free;
	if (ptr->prev_free)
		ptr->prev_free->next_free = ptr->next_free;
		
}

/** Merges neighbouring free pointers.
 * This functions assumes that all arguments
 * passed to it were validated by the caller.
 * \param ptr A pointer to the metadata to be merged with its neighbours.
 * \param arena A pointer to the arena in use. */
static inline void merge_free_ptrs(ptr_t *ptr, arena_t *arena) {
	if (ptr->next && !ptr->next->is_valid) {
		remove_from_free_list(ptr->next, arena);
		size_t new_total_size = ptr->total_size + ptr->next->total_size;
		if (ptr->next->next) {
			ptr->next->next->prev = ptr;
			ptr->next = ptr->next->next;
		}
		remove_from_free_list(ptr, arena);
		ptr->total_size = new_total_size;
		add_to_free_list(ptr, arena);
	}
	if (ptr->prev && !ptr->prev->is_valid) {
		remove_from_free_list(ptr, arena);
		size_t new_total_size = ptr->prev->total_size + ptr->total_size;
		if (ptr->next) {
			ptr->prev->next = ptr->next;
			ptr->next->prev = ptr->prev;
		}
		remove_from_free_list(ptr->prev, arena);
		ptr->prev->total_size = new_total_size;
		add_to_free_list(ptr->prev, arena);
	}
		
}

#endif
