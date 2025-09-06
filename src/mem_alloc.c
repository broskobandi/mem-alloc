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
 * Static inline helper functions
 ****************************************************************************/

// static inline ptr_t *use_arena(size_t size) {
// }

/*****************************************************************************
 * Globals
 ****************************************************************************/

/** Thread local static global instance of the arena. 
 * Each thread gets its own copy of this eliminating the need
 * for mutex locks. */
_Thread_local static arena_t g_arena;
// extern arena_t g_arena;

/*****************************************************************************
 * Definitions of functions declared in the public header.
 ****************************************************************************/

/** Allocate memory of 'size' bytes.
 * 'size' represents the memory size requested by the user. Internally,
 * additional memory is allocated to account for alignment and metadata size.
 * The returned pointer stores the address of the allocated data that is owned
 * by the user (or NULL on failure) and not that of the pointer metadata.
 * The returned pointer (assuming successful allocation) is therefore only
 * guaranteed to be safe to access through pointer arithmetic techniques
 * within the requested allocation size. This function first evaluates whether
 * the total allocation size fits in the arena. If so, the free list or
 * the arena itself (in this order of preference) is used, if not, or if
 * the sum of the total allocation size and the current arena offset is 
 * greater than the arena size, mmap() is used to map a new memory block
 * specifically fir this allocation. 
 * \param The (minimum) size to be allocated.
 * \return A pointer to the allocated data or NULL on failure. */
#include <stdio.h>
void *mem_alloc(size_t size) {
	/* The total size to be allocated. */
	size_t total_size = DATA_OFFSET + ROUNDUP(size);

	/* Allocation metadata to be initialized below. */
	ptr_t *ptr = NULL;

	/* Store 'ptr' in the arena if possible. */
	if (g_arena.offset + total_size <= ARENA_SIZE) {

		/* Get reference to the current free ptr tail of 
		 * the appropriate size class. */
		ptr_t **tail = &g_arena.free_ptr_tails[PTR_SIZE_CLASS(size)];

		/* If '*tail' is not NULL, assign it to 'ptr' and  update
		 * the free ptr tail otherwise, allocate a new memory block
		 * directly in the arena. */
		if (*tail) {
			ptr = *tail;
			if (!(*tail)->prev_free) {
				*tail = NULL;
			} else {
				(*tail)->prev_free->next_free =
					(*tail)->next_free;
			}
		} else {
			ptr = (ptr_t*)&g_arena.buff[g_arena.offset];
			if (g_arena.ptr_list_tail) {
				g_arena.ptr_list_tail->next_valid = ptr;
				ptr->prev_valid = g_arena.ptr_list_tail;
			}
			g_arena.ptr_list_tail = ptr;
			g_arena.offset += total_size;
		}
	} 

	/* Use mmap if the pointer couldn't be allocated in the arena.
	 * Return NULL if the allocation fails.*/
	if (!ptr) {
		ptr = MMAP(total_size);
		if (ptr) {
			ptr->is_mmap = true;
		} else {
			return NULL;
		}
	}

	// I wanted to get rid of this for some reason. Why?
	
	/* The segment below is only executed if 'ptr' was successfully
	 * allocated. */

	/* Initialize 'ptr'. */
	ptr->data = (void*)((unsigned char*)ptr + DATA_OFFSET);
	// printf("%p\n", (unsigned char*)ptr->data - DATA_OFFSET);
	ptr->total_size = total_size;
	ptr->next_free = NULL;
	ptr->prev_free = NULL;
	ptr->is_valid = true;

	/* Return a pointer to the user owned data. */
	return ptr->data;
}

/** Frees the memory pointed to by 'ptr'.
 * Marks the memory region pointed to by 'ptr' ready to be reused if the 
 * memory was allocated in the arena or calls munmap() to unmap the memory
 * if the memory was allocated using mmap().
 * \param ptr The pointer to the memory to be freed.*/
void mem_free(void *ptr) {
	/* Return if ptr is NULL. */
	if (!ptr) return;

	/* Get metadata. */
	ptr_t *p = (ptr_t*)((unsigned char*)ptr - DATA_OFFSET);

	/* Check if the memory was allocated using mmap(). */
	if (p->is_mmap) {
		munmap(p, p->total_size);
		return;
	}

	/* Find size class */
	size_t size = p->total_size - DATA_OFFSET;
	ptr_t *free_ptr_tail = g_arena.free_ptr_tails[PTR_SIZE_CLASS(size)];

	/* Update the appropriate free list. */
	if (!free_ptr_tail) {
		g_arena.free_ptr_tails[PTR_SIZE_CLASS(size)] = p;
	} else {
		free_ptr_tail->next_free = p;
		p->prev_free = free_ptr_tail;
	}

	/* Update ptr state. */
	p->is_valid = false;
}

void *mem_realloc(void *ptr) {
	/* Return if ptr is NULL. */
	if (!ptr) return NULL;

	/* Get metadata. */
	ptr_t *p = (ptr_t*)((unsigned char*)ptr - DATA_OFFSET);

	/* Check if the memory was allocated using mmap(). */
	if (p->is_mmap) {
		munmap(p, p->total_size);
		return NULL;
	}

	/* Find size class */
	// size_t size = p->total_size - DATA_OFFSET;
	// ptr_t *free_ptr_tail = g_arena.free_ptr_tails[PTR_SIZE_CLASS(size)];


	/* Check if realloc can be done in place. */

	return NULL; // just so it compiles
}

/*****************************************************************************
 * Test helpers
 ****************************************************************************/

arena_t *get_arena() {
	return &g_arena;
}

void *arena_buff_ptr();
