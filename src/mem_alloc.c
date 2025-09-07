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
	/* Init ptr */
	ptr_t *ptr = *free_tail;
	ptr->is_valid = true;
	ptr->next_free = NULL;
	ptr->prev_free = NULL;

	/* Update free list */
	if (*free_tail) {
		if ((*free_tail)->prev_free) {
			*free_tail = (*free_tail)->prev_free;
			(*free_tail)->next_free = NULL;
		} else {
			*free_tail = NULL;
		}
	}
	return ptr->mem;
}

static inline void *use_arena(size_t total_size) {
	/* Init ptr */
	ptr_t *ptr = (ptr_t*)&g_arena.buff[g_arena.offset];
	g_arena.offset += total_size;
	ptr->next_in_arena = NULL;
	ptr->next_free = NULL;
	ptr->prev_free = NULL;
	ptr->is_valid = true;
	ptr->is_mmap = false;
	ptr->total_size = total_size;
	ptr->mem = (void*)((unsigned char*)ptr + MEM_OFFSET);

	/* Update ptr list. */
	if (g_arena.ptrs_tail) {
		g_arena.ptrs_tail->next_in_arena = ptr;
		ptr->prev_in_arena = g_arena.ptrs_tail;
		g_arena.ptrs_tail = ptr;
	}
	return ptr->mem;
}

static inline void *use_mmap(size_t total_size) {
	/* Init ptr */
	size_t updated_total_size = ROUNDUP(total_size, (size_t)getpagesize());
	ptr_t *ptr = MMAP(updated_total_size);
	if (!ptr) return NULL;
	ptr->prev_in_arena = NULL;
	ptr->next_in_arena = NULL;
	ptr->next_free = NULL;
	ptr->prev_free = NULL;
	ptr->is_valid = true;
	ptr->is_mmap = true;
	ptr->total_size = updated_total_size;
	ptr->mem = (void*)((unsigned char*)ptr + MEM_OFFSET);
	return ptr->mem;
}

static inline void add_to_free_list(ptr_t *ptr) {
	ptr_t **tail = &g_arena.free_ptr_tails[SIZE_CLASS(ptr->total_size)];
	if (*tail) {
		(*tail)->next_free = ptr;
		ptr->prev_free = (*tail);
	}
	*tail = ptr;
}

static inline void remove_from_free_list(ptr_t *ptr) {
	if (ptr->next_free)
		ptr->next_free->prev_free = ptr->prev_free;
	if (ptr->prev_free)
		ptr->prev_free->next_free = ptr->next_free;
}

static inline void merge_neighbouring_ptrs(ptr_t *ptr) {
	if (!ptr->next_in_arena->is_valid) {
		remove_from_free_list(ptr->next_in_arena);
		ptr->total_size += ptr->next_in_arena->total_size;
		ptr->next_in_arena = ptr->next_in_arena->next_in_arena;
	}
	if (!ptr->prev_in_arena->is_valid) {
		remove_from_free_list(ptr->prev_in_arena);
		ptr->total_size += ptr->prev_in_arena->total_size;
		ptr->prev_in_arena = ptr->prev_in_arena->prev_in_arena;
	}
}

static inline void *realloc_in_place(ptr_t *ptr, size_t size) {
	return ptr->total_size >= MEM_OFFSET + ROUNDUP(size, MIN_ALLOC_SIZE) ?
		ptr->mem : NULL;
}

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
	if (!free_tail)
		mem = use_free_list(free_tail);

	/* Otherwise, allocate in the arena, if size fits */
	if (!mem && g_arena.offset + total_size < ARENA_SIZE)
		mem = use_arena(total_size);

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

	/* Set ptr invalid */
	ptr->is_valid = false;

	/* If the allocation was made with mmap(), use munmap(),
	 * otherwise, handle arena memory. */
	if (ptr->is_mmap) {
		/* Set this to false in case the ptr gets mistakenly
		 * dereferenced in the future. */
		munmap(ptr, ptr->total_size);
	} else {
		add_to_free_list(ptr);
		merge_neighbouring_ptrs(ptr);
	}
}

void *mem_realloc(void *mem, size_t size) {
	/* Validate input. */
	if (!mem) return NULL;

	/* Validate ptr */
	ptr_t *ptr = PTR_META(mem);
	if (!ptr->is_valid) return NULL;

	/* The memory to be returned. */
	void *new_mem = NULL;

	/* Attempt to realloc in place. */
	new_mem = realloc_in_place(ptr, size);

	/* Otherwise, attempt to create a new allocation and copy the data. */
	if (!new_mem) {
		new_mem = mem_alloc(size);
		if (!new_mem) return NULL;
		ptr_t *new_ptr = PTR_META(new_mem);
		size_t size_to_copy =
			ptr->total_size <= new_ptr->total_size ? 
			ptr->total_size : new_ptr->total_size;
		memcpy(new_mem, mem, size_to_copy);
	}

	return new_mem;
}

/*****************************************************************************
 * Helpers for the test utility
 ****************************************************************************/

arena_t *get_arena() {
	return &g_arena;
}
