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
 * Definitions of functions declared in the public header.
 ****************************************************************************/

void *mem_alloc(size_t size) {
	/* Get total allocation size. */
	size_t total_size = MEM_OFFSET + ROUNDUP(size, MIN_ALLOC_SIZE);

	/* A pointer to the user-owned memory to return. */
	void *mem = NULL;

	/* Allocate in free list, if exists. */
	ptr_t **free_tail = &g_arena.free_ptr_tails[SIZE_CLASS(total_size)];
	if (*free_tail)
		mem = use_free_list(free_tail);

	/* Otherwise, allocate in the arena, if size fits */
	if (!mem && g_arena.offset + total_size < ARENA_SIZE)
		mem = use_arena(total_size, &g_arena);

	/* Otherwise, attempt to allocate with mmap(). */
	if (!mem)
		mem = use_mmap(total_size);

	return mem;
}

void mem_free(void *mem) {
	/* Validate input. */
	if (!mem) return;

	/* Validate ptr. */
	ptr_t *ptr = PTR_META(mem);
	if (!ptr->is_valid) return;

	/* If the allocation was made with mmap(), use munmap(),
	 * otherwise, handle arena memory. */
	if (ptr->is_mmap) {
		/* Set this to false in case the ptr gets mistakenly
		 * dereferenced in the future. */
		ptr->is_valid = false;
		munmap(ptr, ptr->total_size);
	} else {
		add_to_free_list(ptr, &g_arena);
		merge_neighbouring_ptrs(ptr, &g_arena);
	}
}

void *mem_realloc(void *mem, size_t size) {
	/* Validate input. */
	if (!mem) return NULL;

	/* Validate ptr */
	ptr_t *ptr = PTR_META(mem);
	if (!ptr->is_valid) return NULL;

	/* Calculate total size */
	size_t total_size = MEM_OFFSET + ROUNDUP(size, MIN_ALLOC_SIZE);

	/* Realloc in place, if possible. */
	if (ptr->total_size <= total_size)
		return ptr->mem;

	/* Otherwise, attempt to create a new allocation and copy the data. */
	void *new_mem = mem_alloc(size);
	if (!new_mem) return NULL;
	ptr_t *new_ptr = PTR_META(new_mem);
	size_t size_to_copy =
		ptr->total_size <= new_ptr->total_size ? 
		ptr->total_size : new_ptr->total_size;
	memcpy(new_mem, mem, size_to_copy);

	return new_mem;
}
