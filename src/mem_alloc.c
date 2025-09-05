#include "mem_alloc_utils.h"
#include "mem_alloc.h"
#include <stdalign.h>
#include <stdbool.h>
#include <sys/mman.h>

#define NUM_ARENAS 32
#define ROUNDUP(size)\
	(((size) + alignof(max_align_t) - 1) & ~(alignof(max_align_t) - 1))
#define DATA_OFFSET\
	ROUNDUP(sizeof(ptr_t))
#define MIN_ALLOC_SIZE alignof(max_align_t)
#define ARENA_SIZE 1024LU * 128
#define NUM_PTR_SIZE_CLASSES\
	(ARENA_SIZE - DATA_OFFSET) / MIN_ALLOC_SIZE

typedef struct ptr ptr_t;
struct ptr {
	void *data;
	size_t total_mem;
	ptr_t *next_free;
	ptr_t *prev_free;
	bool is_valid;
	bool is_mmap;
};

typedef struct arena {
	alignas(max_align_t) unsigned char buff[ARENA_SIZE];
	ptr_t *free_ptr_tails[NUM_PTR_SIZE_CLASSES];
	size_t offset;
} arena_t;
#define PTR_SIZE_CLASS(size)\
	(size) / MIN_ALLOC_SIZE 

_Thread_local static arena_t g_arena;

void *mem_alloc(size_t size) {
}

void mem_free(void *ptr) {
}

void *mem_realloc(void *ptr) {
}
