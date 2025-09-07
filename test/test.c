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
	ASSERT(arena.offset == total_size);
	ASSERT(!arena.free_ptr_tails[SIZE_CLASS(total_size)]);
	ASSERT(arena.ptrs_tail == ptr);
	ASSERT((ptr_t*)arena.buff == ptr);

	void *mem2 = use_arena(total_size, &arena);
	ptr_t *ptr2 = PTR_META(mem2);
	ASSERT(ptr2->prev_in_arena == ptr);
	ASSERT(ptr->next_in_arena == ptr2);
	ASSERT(arena.ptrs_tail == ptr2);
}

void test_add_to_free_list() {
	arena_t arena = {0};
	size_t total_size = MEM_OFFSET + ROUNDUP(sizeof(int), MIN_ALLOC_SIZE);
	void *mem = use_arena(total_size, &arena);
	ptr_t *ptr = PTR_META(mem);
	add_to_free_list(ptr, &arena);
	ptr_t *tail = arena.free_ptr_tails[SIZE_CLASS(total_size)];
	ASSERT(tail == ptr);
}

void test_use_free_list() {
	{ // 
	}
}

int main(void) {
	test_use_arena();
	test_add_to_free_list();
	test_use_free_list();

	test_print_results();
	return 0;
}
