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
	size_t total_size_internal = ROUNDUP(total_size, (size_t)getpagesize());
	void *mem = use_mmap(total_size);
	ASSERT(mem);
	ptr_t *ptr = PTR(mem);
	ASSERT(ptr->mem == mem);
	ASSERT(ptr->is_mmap == true);
	ASSERT(ptr->total_size == total_size_internal);
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

void test_use_free_list() {
	arena_t arena = {0};
	const size_t SIZE = ARENA_SIZE / 32;
	size_t total_size = MEM_OFFSET + ROUNDUP(SIZE, MIN_ALLOC);
	void *mem = use_arena(total_size, &arena);
	ptr_t *ptr = PTR(mem);
	add_to_free_list(ptr, &arena);
	void *mem2 = use_arena(total_size, &arena);
	ptr_t *ptr2 = PTR(mem2);
	add_to_free_list(ptr2, &arena);
	void *mem3 = use_arena(total_size, &arena);
	ptr_t *ptr3 = PTR(mem3);
	add_to_free_list(ptr3, &arena);

	void *mem4 = use_free_list(total_size, &arena);
	ptr_t *ptr4 = PTR(mem4);

	ASSERT(ptr4 == ptr3);
	ASSERT(ptr4->prev_free == ptr2);
	ASSERT(ptr2->next_free == ptr3);
	ASSERT(arena.free_ptr_tails[SIZE_CLASS(SIZE)] == ptr2);

	void *mem5 = use_free_list(total_size, &arena);
	ptr_t *ptr5 = PTR(mem5);

	ASSERT(ptr5 == ptr2);
	ASSERT(ptr5->prev_free == ptr);
	ASSERT(ptr->next_free == ptr2);
	ASSERT(arena.free_ptr_tails[SIZE_CLASS(SIZE)] == ptr);

	void *mem6 = use_free_list(total_size, &arena);
	ptr_t *ptr6 = PTR(mem6);

	ASSERT(ptr6 == ptr);
	ASSERT(!ptr6->prev_free);
	ASSERT(!arena.free_ptr_tails[SIZE_CLASS(SIZE)]);
}

void test_mem_alloc() {
	{ // Use arena
		RESET_ARENA;
		const size_t SIZE = ARENA_SIZE / 32;
		size_t total_size = MEM_OFFSET + ROUNDUP(SIZE, MIN_ALLOC);
		void *mem = mem_alloc(SIZE);
		ptr_t *ptr = PTR(mem);
		ASSERT(ptr->mem == mem);
		ASSERT(global_arena());
		ASSERT(global_arena()->ptrs_tail == ptr);
		ASSERT(ptr->total_size == total_size);

		void *mem2 = mem_alloc(SIZE);
		ptr_t *ptr2 = PTR(mem2);

		ASSERT(ptr2->prev == ptr);
		ASSERT(ptr->next = ptr2);
		ASSERT(global_arena()->ptrs_tail == ptr2);
	}
	{ // Use mmap
		RESET_ARENA;
		const size_t SIZE = ARENA_SIZE * 2;
		size_t total_size = MEM_OFFSET + ROUNDUP(SIZE, MIN_ALLOC);
		size_t total_size_internal = ROUNDUP(total_size, (size_t)getpagesize());
		void *mem = mem_alloc(SIZE);
		ASSERT(mem);
		ptr_t *ptr = PTR(mem);
		ASSERT(ptr->is_mmap);
		ASSERT(!global_arena()->ptrs_tail);
		ASSERT(ptr->total_size == total_size_internal);
		ASSERT(ptr->mem == mem);
	}
	{ // Use free list
		RESET_ARENA;
		const size_t SIZE = ARENA_SIZE / 32;
		void *mem = mem_alloc(SIZE);
		void *mem2 = mem_alloc(SIZE);
		ptr_t *ptr = PTR(mem);
		ptr_t *ptr2 = PTR(mem2);
		add_to_free_list(ptr, global_arena());
		add_to_free_list(ptr2, global_arena());
		ASSERT(global_arena()->free_ptr_tails[SIZE_CLASS(SIZE)] == ptr2);

		void *mem3 = mem_alloc(SIZE);
		void *mem4 = mem_alloc(SIZE);

		ASSERT(!global_arena()->free_ptr_tails[SIZE_CLASS(SIZE)]);
		ASSERT(mem3 == mem2);
		ASSERT(mem4 == mem);
	}
}

int main(void) {
	test_use_arena();
	test_use_mmap();
	test_add_to_free_list();
	test_use_free_list();
	test_mem_alloc();

	test_print_results();
	return 0;
}
