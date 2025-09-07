#include <test.h>
#include "mem_alloc_utils.h"

TEST_INIT;

void test_use_arena() {
	arena_t arena = {0};
	size_t total_size = MEM_OFFSET + ROUNDUP(sizeof(int), MIN_ALLOC_SIZE);
	void *mem = use_arena(total_size, &arena);
	ptr_t *ptr = PTR_META(mem);
	ASSERT(mem == ptr->mem);
	ASSERT(!ptr->is_mmap);
	ASSERT(ptr->total_size == total_size);
	ASSERT(ptr->is_valid);
	ASSERT(!ptr->prev_in_arena);
	ASSERT(!ptr->next_in_arena);
	ASSERT(!ptr->prev_free);
	ASSERT(!ptr->next_free);
}

void test_use_free_list() {
	{ // 
	}
}

int main(void) {
	test_use_arena();
	test_use_free_list();

	test_print_results();
	return 0;
}
