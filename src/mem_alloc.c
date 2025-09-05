#include "mem_alloc_utils.h"

void *mem_alloc(size_t size) {
	ptr_t *ptr = NULL;
	size_t size_to_alloc = DATA_OFFSET + ROUNDUP(size);
	if (g_arena.offset + size_to_alloc <= ARENA_SIZE) {
		if (g_arena.free_ptr_tails[PTR_SIZE_CLASS(size_to_alloc)]) {
			ptr = g_arena.free_ptr_tails[PTR_SIZE_CLASS(size_to_alloc)];
			if (ptr->prev_free)
				ptr->prev_free->next_free = ptr->next_free;
			if (ptr->next_free)
				ptr->next_free->prev_free = ptr->prev_free;
		} else {
			ptr = (ptr_t*)((unsigned char*)g_arena.buff + g_arena.offset);
		}
	}
	ptr = mmap(NULL, size_to_alloc, PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (!ptr) return NULL;
	ptr->data = (unsigned char*)ptr + DATA_OFFSET;
	ptr->total_mem = size_to_alloc;
	ptr->is_valid = true;
	ptr->next_free = NULL;
	ptr->prev_free = NULL;
	return ptr->data;
}

// void mem_free(void *ptr) {
// 	if (!ptr) return;
// 	ptr_t *p = (ptr_t*)((unsigned char*)ptr - DATA_OFFSET);
// 	if (p.)
// }
