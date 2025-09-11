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

/** \file src/mem_alloc.c
 * \brief Implementation of the mem_alloc library.
 * \details This file contains the definitions of public funcions, galobals,
 * and debug macros for the mem_alloc library. */

#include "mem_alloc_private.h"

/******************************************************************************
 * Global variables
 *****************************************************************************/

/** The global arena to be used as the default allocator. */
_Thread_local static arena_t g_arena;

/******************************************************************************
 * Macro definitions
 *****************************************************************************/

#ifndef NDEBUG
#include <stdio.h>
_Thread_local static int g_is_arena_full;
_Thread_local static int g_is_arena_init;
static inline void warn_arena_init() {
	if (!g_is_arena_init) {
		g_is_arena_init = 1;
		printf("[MEM_ALLOC WARNING]:\n");
		printf("\tFirst use of arena of size %luKB\n", ARENA_SIZE/1024);
	}
}
static inline void warn_arena_full() {
	if (!g_is_arena_full) {
		g_is_arena_full = 1;
		printf("[MEM_ALLOC WARNING]:\n");
		printf("\tUsing mmap from now on.\n");
	}
}
#define WARN_ARENA_INIT\
	warn_arena_init()
#define WARN_ARENA_FULL\
	warn_arena_full()
#else
#define WARN_ARENA_FULL
#define WARN_ARENA_INIT
#endif

/******************************************************************************
 * Helpers for the test utility
 *****************************************************************************/

/** For the test utility: Returns a pointer to the global arena.
 * \return A pointer to the global arena. */
arena_t *global_arena() {
	return &g_arena;
}

/** For the test utility: Resets the global arena to its default 
 * sate. */
void reset_global_arena() {
	memset(&g_arena, 0, sizeof(arena_t));
}

/******************************************************************************
 * Public function definitions
 *****************************************************************************/

/** Allocates memory of 'size' bytes in an internal static buffer or in the 
 * heap if the buffer is full.
 * \param size The number of bytes to allocate.
 * \return A pointer to the allocated memory or NULL on failure. */
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
		WARN_ARENA_INIT;
		return use_arena(total_size, &g_arena);
	}
	return NULL;
}

/** Deallocates memory pointed to by 'ptr'.
 * \param ptr A pointer to the memory to be freed.
 * \return 0 if heap memory was successfully unmapped,
 * 1 if the memory was at the end of the internal buffer and therefore 
 * only the offset was updated, 2 if the memory was in the middle of the
 * buffer and is now added to the free list, -1 on failure. */
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

/** Reallocates the allocated memory pointed to by 'ptr' 
 * by either shrinking it or expanding it to be 'size' number of bytes. 
 * \param ptr The pointer to the memory to be resized.
 * \param size The size of the new allocation in bytes. 
 * \return A pointer to the new memory location (which may be 
 * the same as the pointer passed to it in case in-place reallocation was
 * possible) or NULL on failure. */
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
