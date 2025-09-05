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
#include "mem_alloc.h"

#include <stdalign.h> /* for alignof(max_align_t) */
#include <stdbool.h>
#include <sys/mman.h> /* for mmap() and munmap() */

/*****************************************************************************
 * Macro helpers
 ****************************************************************************/

/** Rounds up 'size' to the nearest multiple of alignof(max_align_t) */
#define ROUNDUP(size)\
	(((size) + alignof(max_align_t) - 1) & ~(alignof(max_align_t) - 1))

/** Calculates data offset from the start of the pointer metadata. */
#define DATA_OFFSET ROUNDUP(sizeof(ptr_t))

/** The minimum allocation size which is the max alignment on
 * the platform. */
#define MIN_ALLOC_SIZE alignof(max_align_t)

/** The number of bytes that can be stored in the arena. 
 * This can be overridden by recompiling the library with 
 * -DARENA_SIZE_CUSTOM=<custom_size> compiler flag. */
#ifndef ARENA_SIZE_CUSTOM
#define ARENA_SIZE 1024LU * 128
#else
#define ARENA_SIZE ARENA_SIZE_CUSTOM
#endif

/** The number every possible allocation sizes. */
#define NUM_PTR_SIZE_CLASSES (ARENA_SIZE - DATA_OFFSET) / MIN_ALLOC_SIZE

/** Calculates size class that corresponds with 'size'. */
#define PTR_SIZE_CLASS(size) (size) / MIN_ALLOC_SIZE 

/** Shorthand for calling mmap with the standard arguments. */
#define MMAP(size)\
	mmap(\
		NULL,\
		(size),\
		PROT_WRITE | PROT_READ,\
		MAP_PRIVATE | MAP_ANONYMOUS,\
		-1,\
		0\
	);

/*****************************************************************************
 * Structs
 ****************************************************************************/

/** Struct for storing metadata for an allocated block of memory. */
typedef struct ptr ptr_t;

/** Struct for storing metadata for the global arena. */
typedef struct arena arena_t;

/** Ptr struct definition. */
struct ptr {
	/** Pointer to the user owned data. */
	void *data;

	/** The (aligned and rounded up) sum of the user requested 
	 * memory size and the metadata size. */
	size_t total_size;

	/** The next free pointer. Only relevant if the pointer was freed. */
	ptr_t *next_free;

	/** The previous free pointer. Only relevant if the pointer was
	 * freed. */
	ptr_t *prev_free;

	/** Boolean indicating whether the pointer is valid or not.
	 * It is set to false when the pointer is freed. Needed
	 * for checking if expansion can be done in place. */
	bool is_valid;

	/** Boolean indicating whether the memory was allocated in the 
	 * arena or in thea heap using mmap().*/
	bool is_mmap;
};

/** Arena struct definition */
struct arena {
	/** Main buffer. */
	alignas(max_align_t) unsigned char buff[ARENA_SIZE];

	/** An array of linked list of all size classes of free pointers. */
	ptr_t *free_ptr_tails[NUM_PTR_SIZE_CLASSES];

	/** The size of the region in the arena that is currently
	 * occupied by either valid or invalid (freed and ready to be reused)
	 * allocations. */
	size_t offset;
};

/*****************************************************************************
 * Globals
 ****************************************************************************/

/** Thread local static global instance of the arena. 
 * Each thread gets its own copy of this eliminating the need
 * for mutex locks. */
_Thread_local static arena_t g_arena;

/*****************************************************************************
 * Static inline helper functions
 ****************************************************************************/

/** Cleans up the appropriate free list after reusing a free pointer.
 * \param tail The current tail of the free list.
 * \param size The size value necessary to determine the appropriate
 * size class. */
// static inline void cleanup_free_list(size_t size) {
// 	/* A pointer to the current free ptr tail. */
// 	ptr_t **tail_ref = &g_arena.free_ptr_tails[PTR_SIZE_CLASS(size)];
//
// 	/* The actual free ptr tail. */
// 	ptr_t *tail = *tail_ref;
//
// 	/* Set the new  */
// 	if (tail->prev_free) {
// 		tail->prev_free->next_free = tail->next_free;
// 	} else {
// 		*tail_ref = NULL;
// 	}
// }

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

	/* The segment below is only executed if 'ptr' was successfully
	 * allocated. */

	/* Initialize 'ptr'. */
	ptr->data = (void*)&ptr[DATA_OFFSET];
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
	if (!ptr) return;
	ptr_t *p = (ptr_t*)((unsigned char*)ptr - DATA_OFFSET);
	if (p->is_mmap) {
		munmap(p, p->total_size);
		return;
	}
	size_t size = p->total_size - DATA_OFFSET;
	ptr_t *free_ptr_tail = g_arena.free_ptr_tails[PTR_SIZE_CLASS(size)];
	if (!free_ptr_tail) {
		g_arena.free_ptr_tails[PTR_SIZE_CLASS(size)] = p;
	} else {
		free_ptr_tail->next_free = p;
		p->prev_free = free_ptr_tail;
	}
	p->is_valid = false;
}

// void *mem_realloc(void *ptr) {
// }
