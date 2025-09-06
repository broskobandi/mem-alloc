#ifndef MEM_ALLOC_UTILS_H
#define MEM_ALLOC_UTILS_H

#include "mem_alloc.h"

#include <stdalign.h> /* for alignof(max_align_t) */
#include <stdbool.h>
#include <sys/mman.h> /* for mmap() and munmap() */
#include <string.h> /* for memcpy() */
#include <unistd.h> /* for getpagesize() */

/*****************************************************************************
 * Macro definitions
 ****************************************************************************/

/** The minimum allocation size which is the max alignment on
 * the platform. */
#define MIN_ALLOC_SIZE alignof(max_align_t)

/** Rounds up 'size' to the nearest multiple of 'to' */
#define ROUNDUP(size, to)\
	(((size) + (to) - 1) & ~((to) - 1))

/** Calculates mem offset from the start of the pointer metadata. */
#define MEM_OFFSET ROUNDUP(sizeof(ptr_t), MIN_ALLOC_SIZE)

/** Calculates memory location of the ptr metadata. */
#define PTR_META(pointer) (ptr_t*)((unsigned char*)(pointer) -  MEM_OFFSET)

/** The number of bytes that can be stored in the arena. 
 * This can be overridden by recompiling the library with 
 * -DARENA_SIZE_CUSTOM=<custom_size> compiler flag. */
#ifndef ARENA_SIZE_CUSTOM
#define ARENA_SIZE 1024LU * 128
#else
#define ARENA_SIZE ARENA_SIZE_CUSTOM
#endif

/** The maximum number of free pointers kept after freeing mmap
 * allocated memory. */
#ifndef MAX_NUM_FREE_MMAP_PTRS_CUSTOM
#define MAX_NUM_FREE_MMAP_PTRS 8
#else
#define MAX_NUM_FREE_MMAP_PTRS MAX_NUM_FREE_MMAP_PTRS_CUSTOM
#endif

/** The number every possible allocation sizes. */
#define NUM_SIZE_CLASSES (ARENA_SIZE - MEM_OFFSET) / MIN_ALLOC_SIZE

/** Calculates size class that corresponds with 'size'. */
#define SIZE_CLASS(total_size) (total_size) / (MEM_OFFSET + MIN_ALLOC_SIZE)

/** Shorthand for calling mmap with the standard arguments. */
#define MMAP(total_size)\
	mmap(\
		NULL,\
		(total_size),\
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
	/** Pointer to the user owned block of memory. */
	void *mem;

	/** The (aligned and rounded up) sum of the user requested 
	 * memory size and the metadata size. */
	size_t total_size;

	/** The next free pointer. Only relevant if the pointer was freed. */
	ptr_t *next_free;

	/** The previous free pointer. Only relevant if the pointer was
	 * freed. */
	ptr_t *prev_free;

	/** Next valid pointer. Only relevant if the pointer is not 
	 * int the free list.*/
	ptr_t *next_valid;

	/** Previous valid pointer. Only relevant if the pointer is not 
	 * int the free list.*/
	ptr_t *prev_valid;

	/** Boolean indicating whether the pointer is valid or not.
	 * It is set to false when the pointer is freed. */
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
	ptr_t *free_ptr_tails[NUM_SIZE_CLASSES];

	/** A list of all the pointers stored (valid or invalid). */
	ptr_t *ptrs_tail;

	/** The size of the region in the arena that is currently
	 * occupied by either valid or invalid (freed and ready to be reused)
	 * allocations. */
	size_t offset;
};

/*****************************************************************************
 * Test helpers
 ****************************************************************************/

/** Returns a pointer to the global arena object.
 * This is to avoid having to declare the static global instance in the 
 * header file. */
arena_t *get_arena();

#endif
