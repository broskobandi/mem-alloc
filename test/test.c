#include "mem_alloc_private.h"
#include <test.h>

TEST_INIT;

void test_use_arena() {
	arena_t arena = {0};
	const size_t SIZE = ARENA_SIZE / 32;
	size_t total_size = MEM_OFFSET + ROUNDUP(SIZE, MIN_ALLOC);
	void *mem = use_arena(total_size, &arena);
	ptr_t *ptr = PTR(mem);
	ASSERT(mem);
	ASSERT(mem == ptr->mem);
	ASSERT(ptr->total_size == MEM_OFFSET + ROUNDUP(SIZE, MIN_ALLOC));
	ASSERT(arena.offset == total_size);
	ASSERT(arena.ptrs_tail == ptr);
	ASSERT(ptr->is_valid == true);
	ASSERT(ptr->is_mmap == false);
	ASSERT(!ptr->next_free);
	ASSERT(!ptr->prev_free);
	ASSERT(!ptr->next);
	ASSERT(!ptr->prev);

	void *mem2 = use_arena(total_size, &arena);
	ptr_t *ptr2 = PTR (mem2);
	ASSERT(ptr2->prev == ptr);
	ASSERT(ptr->next == ptr2);
	ASSERT(arena.ptrs_tail == ptr2);
}

void test_use_mmap() {
	const size_t SIZE = ARENA_SIZE * 2;
	size_t total_size = MEM_OFFSET + ROUNDUP(SIZE, MIN_ALLOC);
	void *mem = use_mmap(total_size);
	ASSERT(mem);
	ptr_t *ptr = PTR(mem);
	ASSERT(ptr->mem == mem);
	ASSERT(ptr->is_mmap == true);
}

void test_add_to_free_list() {
	const size_t SIZE = ARENA_SIZE / 32;
	size_t total_size = MEM_OFFSET + ROUNDUP(SIZE, MIN_ALLOC);
	arena_t arena = {0};
	void *mem = use_arena(total_size, &arena);
	ptr_t *ptr = PTR(mem);
	add_to_free_list(ptr, &arena);
	ptr_t *free_tail = arena.free_ptr_tails[SIZE_CLASS(SIZE)];
	ASSERT(ptr == free_tail);

	void *mem2 = use_arena(total_size, &arena);
	ptr_t *ptr2 = PTR(mem2);
	add_to_free_list(ptr2, &arena);

	ASSERT(ptr2->prev_free == ptr);
	ASSERT(ptr->next_free == ptr2);
	free_tail = arena.free_ptr_tails[SIZE_CLASS(SIZE)];
	ASSERT(free_tail == ptr2);

	const size_t SIZE2 = ARENA_SIZE / 16;
	void *mem3 = use_arena(SIZE2, &arena);
	ptr_t *ptr3 = PTR(mem3);
	add_to_free_list(ptr3, &arena);

	ASSERT(ptr3->prev_free != ptr2);
}

int main(void) {
	test_use_arena();
	test_use_mmap();
	test_add_to_free_list();

	test_print_results();
	return 0;
}
