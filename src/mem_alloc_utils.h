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
#define PTR_META(mem) (ptr_t*)((unsigned char*)(mem) -  MEM_OFFSET)

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

	/** Next ptr in arena. Only relevant if the pointer is not 
	 * int the free list.*/
	ptr_t *next_in_arena;

	/** Previous ptr in arena. Only relevant if the pointer is not 
	 * int the free list.*/
	ptr_t *prev_in_arena;

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
 * Static inline helper functions
 ****************************************************************************/

static inline void *use_free_list(ptr_t **free_tail) {
	/* Init ptr */
	ptr_t *ptr = *free_tail;
	ptr->is_valid = true;

	/* Update free list. */
	if ((*free_tail)->prev_free) {
		*free_tail = (*free_tail)->prev_free;
		(*free_tail)->next_free = NULL;
	} else {
		*free_tail = NULL;
	}

	ptr->next_free = NULL;
	ptr->prev_free = NULL;

	return ptr->mem;
}

static inline void *use_arena(size_t total_size, arena_t *arena) {
	/* Init ptr */
	ptr_t *ptr = (ptr_t*)&arena->buff[arena->offset];
	arena->offset += total_size;
	ptr->next_in_arena = NULL;
	ptr->next_free = NULL;
	ptr->prev_free = NULL;
	ptr->is_valid = true;
	ptr->is_mmap = false;
	ptr->total_size = total_size;
	ptr->mem = (void*)((unsigned char*)ptr + MEM_OFFSET);

	/* Update ptr list. */
	if (arena->ptrs_tail) {
		arena->ptrs_tail->next_in_arena = ptr;
		ptr->prev_in_arena = arena->ptrs_tail;
	}
	arena->ptrs_tail = ptr;
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

static inline void add_to_free_list(ptr_t *ptr, arena_t *arena) {
	ptr->is_valid = false;
	ptr_t **tail = &arena->free_ptr_tails[SIZE_CLASS(ptr->total_size)];
	if (*tail) {
		(*tail)->next_free = ptr;
		ptr->prev_free = (*tail);
	}
	*tail = ptr;
}

static inline void remove_from_free_list(ptr_t *ptr, arena_t *arena) {
	ptr_t **tail = &arena->free_ptr_tails[SIZE_CLASS(ptr->total_size)];
	if (ptr->next_free) {
		ptr->next_free->prev_free = ptr->prev_free;
	} else {
		*tail = ptr->prev_free;
	}
	if (ptr->prev_free)
		ptr->prev_free->next_free = ptr->next_free;
}

static inline void merge_neighbouring_ptrs(ptr_t *ptr, arena_t *arena) {
	if (ptr->next_in_arena && !ptr->next_in_arena->is_valid) {
		remove_from_free_list(ptr->next_in_arena, arena);
		ptr->total_size += ptr->next_in_arena->total_size;
		ptr->next_in_arena = ptr->next_in_arena->next_in_arena;
		ptr->next_in_arena->prev_in_arena = ptr;
		remove_from_free_list(ptr, arena);
		add_to_free_list(ptr, arena);
	}
	if (ptr->prev_in_arena && !ptr->prev_in_arena->is_valid) {
		remove_from_free_list(ptr, arena);
		ptr->prev_in_arena->total_size += ptr->total_size;
		ptr->prev_in_arena->next_in_arena = ptr->next_in_arena;
		ptr->next_in_arena->prev_in_arena = ptr->prev_in_arena;
		remove_from_free_list(ptr->prev_in_arena, arena);
		add_to_free_list(ptr->prev_in_arena, arena);
	}
}

static inline void move_ptr(ptr_t **ptr, size_t size) {
	ptr_t *new_ptr = (ptr_t*)((unsigned char*)*ptr + size);
	memcpy(new_ptr, *ptr, sizeof(ptr_t));
	*ptr = new_ptr;
	(*ptr)->mem = ((unsigned char*)*ptr + MEM_OFFSET);
	if ((*ptr)->next_in_arena)
		(*ptr)->next_in_arena->prev_in_arena = *ptr;
	if ((*ptr)->prev_in_arena)
		(*ptr)->prev_in_arena->next_in_arena = *ptr;
	if ((*ptr)->next_free)
		(*ptr)->next_free->prev_free = *ptr;
	if ((*ptr)->prev_free)
		(*ptr)->prev_free->next_free = *ptr;
}

static inline void *realloc_in_place(
	ptr_t *ptr, size_t total_size, arena_t *arena
) {
	/* If current size is enough */
	if (ptr->total_size >= total_size)
		return ptr->mem;

	ptr_t **next = &ptr->next_in_arena;
	size_t extra_size_needed =
		ROUNDUP(total_size - ptr->total_size, MIN_ALLOC_SIZE);

	/* If next pointer is free and is bigger than needed */
	if (
		*next && !(*next)->is_valid &&
		(*next)->total_size >=
		extra_size_needed + MEM_OFFSET + MIN_ALLOC_SIZE
	) {
		move_ptr(next, extra_size_needed);
		(*next)->total_size -= extra_size_needed;
		remove_from_free_list(*next, arena);
		add_to_free_list(*next, arena);
		ptr->total_size += extra_size_needed;
		return ptr->mem;
	}

	/* If next pointer is free and is just as big as needed */
	if (
		*next && !(*next)->is_valid &&
		(*next)->total_size == extra_size_needed
	) {
		remove_from_free_list(*next, arena);
		ptr->total_size += extra_size_needed;
		arena->offset += extra_size_needed;
		return ptr->mem;
	}

	/* If there is no next pointer. */
	if (!*next && arena->offset + extra_size_needed <= ARENA_SIZE) {
		ptr->total_size += extra_size_needed;
		arena->offset += extra_size_needed;
		return ptr->mem;
	}

	return NULL;
}

#endif
