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

/**
 * \file src/mem_alloc.c
 * \brief Implementation for the mem_alloc library.
 * \details This file contains the definitions of types, structs, 
 * and public functions.
 * */

#include "mem_alloc_utils.h"

/*****************************************************************************
 * Globals
 ****************************************************************************/

/** Thread local static global instance of the arena. */
_Thread_local static arena_t g_arena;

/*****************************************************************************
 * Static inline helper functions
 ****************************************************************************/

static inline void *use_free_list(ptr_t **free_tail) {
	/* Set mem */
	ptr_t *ptr = *free_tail;
	void *mem = ptr->mem;
	ptr->is_valid = true;
	ptr->next_free = NULL;
	ptr->prev_free = NULL;

	/* Cleanup free list. */
	if (ptr->prev_free) {
		ptr->prev_free->next_free = ptr->next_free;
		*free_tail = ptr->prev_free;
	} else {
		*free_tail = NULL;
	}

	return mem;
}

static inline void *allocate_new_mem_in_arena(size_t total_size) {
	/* Set mem */
	ptr_t *ptr = (ptr_t*)&g_arena.buff[g_arena.offset];
	ptr->mem = (void*)((unsigned char*)ptr + MEM_OFFSET);
	void *mem = ptr->mem;
	ptr->is_valid = true;
	ptr->next_free = NULL;
	ptr->prev_free = NULL;
	ptr->is_mmap = false;
	ptr->total_size = total_size;

	/* Update ptr list */
	if (g_arena.ptrs_tail) {
		g_arena.ptrs_tail->next_valid = ptr;
		ptr->prev_valid = g_arena.ptrs_tail;
	}
	g_arena.ptrs_tail = ptr;

	return mem;
}

static inline void *use_arena(size_t total_size) {
	void *mem = NULL;

	/* Attempt to use free list or fallback to making a new allocation
	 * in the arena. */
	ptr_t **free_tail = &g_arena.free_ptr_tails[SIZE_CLASS(total_size)];
	if (*free_tail) {
		mem = use_free_list(free_tail);
	} else {
		mem = allocate_new_mem_in_arena(total_size);
	}

	return mem;
}

static inline void *use_mmap(size_t total_size) {
	void *mem = NULL;

	/* Round up total size to page size */
	total_size = ROUNDUP(total_size, (size_t)getpagesize());
	ptr_t *ptr = MMAP(total_size);
	if (!ptr) return NULL;
	ptr->mem = (void*)((unsigned char*)ptr + MEM_OFFSET);
	mem = ptr->mem;
	ptr->is_mmap = true;
	ptr->is_valid = true;
	ptr->total_size = total_size;
	ptr->prev_valid = NULL;
	ptr->next_valid = NULL;
	ptr->next_free = NULL;
	ptr->prev_free = NULL;

	return mem;
}

// A free list should be implemented for mmap allocations as well.
static inline void mmap_free(ptr_t *ptr) {
	/* Set this to false to avoid problems if this region is 
	 * mistakenly dereferenced before the OS reuses it. */
	ptr->is_valid = false;

	munmap(ptr, ptr->total_size);
}

static inline void arena_free(ptr_t *ptr) {
	/* Set this to false to avoid problems if this region is 
	 * mistakenly dereferenced before it is reused. */
	ptr->is_valid = false;

	/* If it is the last allocated block of memory in the arena,
	 * simply remove the pointer from the list and 
	 * update offset. Otherwise, update free list. */
	if (g_arena.ptrs_tail == ptr) {
		g_arena.ptrs_tail = ptr->prev_valid;
		g_arena.offset -= ptr->total_size;
		return;
	} else {
		ptr_t **tail =
			&g_arena.free_ptr_tails[SIZE_CLASS(ptr->total_size)];
		if (*tail) {
			(*tail)->next_free = ptr;
			ptr->prev_free = *tail;
		}
		*tail = ptr;
	}
}

/*****************************************************************************
 * Definitions of functions declared in the public header.
 ****************************************************************************/

/** Allocates memory of 'size' bytes.
 * 'size' is the user requested memory allocation size, and as such, it
 * represents the range within which it is guaranteed to be safe to  carry
 * out pointer arithmetic operations over the returned pointer (assuming
 * successful allocation). In actual size of the allocation contains the size
 * of the corresponding metadata and padding. The extent of  the padding
 * depends on whether the allocation is evaluated to be made using the
 * thread-local global static arena or mmap(). The allocation is made using
 * mmap() if the requested memory size (plus metadata and padding size)
 * is greater than ARENA_SIZE, or if the arena is full.
 * All arena allocation requests first attempt to use a free list for
 * increased performance.
 * \param size The number (minimum) of bytes to be allocated.
 * \return A pointer to the allocated memory or NULL on failure. */
void *mem_alloc(size_t size) {
	/* Total allocation size. */
	size_t total_size = MEM_OFFSET + ROUNDUP(size, MIN_ALLOC_SIZE);

	/* Use arena if possible, otherwise, use mmap. */
	if (g_arena.offset + total_size <= ARENA_SIZE) {
		return use_arena(total_size);
	} else {
		return use_mmap(total_size);
	}
}

void mem_free(void *mem) {
	/* Validate arg */
	if (!mem) return;

	/* Get metadata */
	ptr_t *ptr = PTR_META(mem);

	/* Validate pointer */
	if (!ptr || !ptr->is_valid) return;

	/* Free memory based on allocation type */
	if (ptr->is_mmap) {
		mmap_free(ptr);
	} else {
		arena_free(ptr);
	}
}

/*****************************************************************************
 * Helpers for the test utility
 ****************************************************************************/

arena_t *get_arena() {
	return &g_arena;
}
