#include "mem_alloc_utils.h"

void *mem_alloc(size_t size) {
	ptr_t *ptr = NULL;
	size_t size_to_alloc = DATA_OFFSET + ROUNDUP(size);
	if (g_arena.offset + size_to_alloc <= ARENA_SIZE) {
		ptr = (ptr_t*)((unsigned char*)g_arena.buff + g_arena.offset);
	}
	ptr = mmap(NULL, size_to_alloc, PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (!ptr) return NULL;
	ptr->data = (unsigned char*)ptr + DATA_OFFSET;
	ptr->total_mem = size_to_alloc;
	return ptr->data;
}
