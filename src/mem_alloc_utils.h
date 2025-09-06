#ifndef MEM_ALLOC_UTILS_H
#define MEM_ALLOC_UTILS_H

#include "mem_alloc.h"

#include <stdalign.h> /* for alignof(max_align_t) */
#include <stdbool.h>
#include <sys/mman.h> /* for mmap() and munmap() */
#include <string.h> /* for memcpy */

/*****************************************************************************
 * Macro definitions
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
 * Test helpers
 ****************************************************************************/

/** Returns a pointer to the global arena object.
 * This is to avoid having to declare the static global instance in the 
 * header file. */
arena_t *get_arena();

#endif
