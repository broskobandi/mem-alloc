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
	ASSERT(!ptr->is_valid);

	void *mem2 = use_arena(total_size, &arena);
	ptr_t *ptr2 = PTR_META(mem2);
	add_to_free_list(ptr2, &arena);
	tail = arena.free_ptr_tails[SIZE_CLASS(total_size)];
	ASSERT(tail == ptr2);
	ASSERT(ptr2->prev_free == ptr);
	ASSERT(ptr->next_free == ptr2);
	ASSERT(!ptr2->is_valid);

	total_size = MEM_OFFSET + ROUNDUP(total_size * 2, MIN_ALLOC_SIZE);

	void *mem3 = use_arena(total_size, &arena);
	ptr_t *ptr3 = PTR_META(mem3);
	add_to_free_list(ptr3, &arena);

	void *mem4 = use_arena(total_size, &arena);
	ptr_t *ptr4 = PTR_META(mem4);
	add_to_free_list(ptr4, &arena);

	ASSERT(ptr2->next_free != ptr3);
	ASSERT(ptr3->next_free == ptr4);
	ASSERT(ptr4->prev_free == ptr3);
}

void test_use_free_list() {
	arena_t arena = {0};
	size_t total_size = MEM_OFFSET + ROUNDUP(sizeof(int), MIN_ALLOC_SIZE);
	void *mem = use_arena(total_size, &arena);
	ptr_t *ptr = PTR_META(mem);
	add_to_free_list(ptr, &arena);

	void *mem2 = use_arena(total_size, &arena);
	ptr_t *ptr2 = PTR_META(mem2);
	add_to_free_list(ptr2, &arena);

	ptr_t **free_tail = &arena.free_ptr_tails[SIZE_CLASS(total_size)];
	ASSERT(*free_tail == ptr2);
	ASSERT(ptr2->prev_free == ptr);

	void *mem3 = use_free_list(free_tail);
	ptr_t *ptr3 = PTR_META(mem3);
	ASSERT(ptr3 == ptr2);
	ASSERT(mem3 == mem2);
	free_tail = &arena.free_ptr_tails[SIZE_CLASS(total_size)];
	ASSERT(*free_tail == ptr);
	ASSERT(!ptr->next_free);

	void *mem4 = use_free_list(free_tail);
	ptr_t *ptr4 = PTR_META(mem4);
	ASSERT(ptr4 == ptr);
	ASSERT(mem4 == mem);
	free_tail = &arena.free_ptr_tails[SIZE_CLASS(total_size)];
	ASSERT(!*free_tail);
}

void test_remove_from_free_list() {
	arena_t arena = {0};
	size_t total_size = MEM_OFFSET + ROUNDUP(sizeof(int), MIN_ALLOC_SIZE);

	void *mem = use_arena(total_size, &arena);
	ptr_t *ptr = PTR_META(mem);
	add_to_free_list(ptr, &arena);

	void *mem2 = use_arena(total_size, &arena);
	ptr_t *ptr2 = PTR_META(mem2);
	add_to_free_list(ptr2, &arena);

	void *mem3 = use_arena(total_size, &arena);
	ptr_t *ptr3 = PTR_META(mem3);
	add_to_free_list(ptr3, &arena);

	remove_from_free_list(ptr2, &arena);
	ASSERT(ptr->next_free == ptr3);
	ASSERT(ptr3->prev_free == ptr);

	ptr_t **tail = &arena.free_ptr_tails[SIZE_CLASS(total_size)];

	remove_from_free_list(ptr3, &arena);
	ASSERT(ptr == *tail);
	ASSERT(!ptr->next_free);
	ASSERT(!ptr->prev_free);

	remove_from_free_list(ptr, &arena);
	ASSERT(!*tail);
}

void test_megre_neighbouring_ptrs() {
	arena_t arena = {0};
	size_t total_size = MEM_OFFSET + ROUNDUP(sizeof(int), MIN_ALLOC_SIZE);

	void *mem = use_arena(total_size, &arena);
	ptr_t *ptr = PTR_META(mem);
	add_to_free_list(ptr, &arena);

	void *mem2 = use_arena(total_size, &arena);
	ptr_t *ptr2 = PTR_META(mem2);
	add_to_free_list(ptr2, &arena);

	void *mem3 = use_arena(total_size, &arena);
	ptr_t *ptr3 = PTR_META(mem3);
	add_to_free_list(ptr3, &arena);

	void *mem4 = use_arena(total_size, &arena);
	ptr_t *ptr4 = PTR_META(mem4);

	merge_neighbouring_ptrs(ptr2, &arena);
	ASSERT(ptr->total_size == total_size * 3);
	ASSERT(ptr->next_in_arena == ptr4);
	ASSERT(ptr4->prev_in_arena == ptr);
	ASSERT(arena.free_ptr_tails[SIZE_CLASS(ptr->total_size)] == ptr);
}

void test_move_ptr() {
	arena_t arena = {0};

	size_t big_size = (MEM_OFFSET + MIN_ALLOC_SIZE) * 4;
	size_t small_size = MEM_OFFSET + MIN_ALLOC_SIZE;
	size_t move_size = MIN_ALLOC_SIZE;

	void *mem = use_arena(big_size, &arena);
	ptr_t *ptr = PTR_META(mem);
	void *mem2 = use_arena(small_size, &arena);
	ptr_t *ptr2 = PTR_META(mem2);
	void *mem3 = use_arena(big_size, &arena);
	ptr_t *ptr3 = PTR_META(mem3);

	move_ptr(&ptr2, move_size);
	ASSERT(ptr->next_in_arena == ptr2);
	ASSERT(ptr2->prev_in_arena == ptr);
	ASSERT(ptr3->prev_in_arena == ptr2);
	ASSERT(ptr2->next_in_arena == ptr3);
}

void test_realloc_in_place() {
	{ // Current size if enough
		arena_t arena = {0};
		size_t original_size = MEM_OFFSET + MIN_ALLOC_SIZE;
		size_t new_size = original_size;
		void *mem = use_arena(original_size, &arena);
		ptr_t *ptr = PTR_META(mem);
		void *new_mem = realloc_in_place(ptr, new_size, &arena);
		ptr_t *new_ptr = PTR_META(new_mem);
		ASSERT(new_ptr->total_size == ptr->total_size);
	}
	{ // Next ptr is free and is bigger than needed
		arena_t arena = {0};
		size_t original_size = MEM_OFFSET + MIN_ALLOC_SIZE;
		size_t new_size = original_size * 2;
		size_t extra_size_needed = new_size - original_size;
		void *mem = use_arena(original_size, &arena);
		ptr_t *ptr = PTR_META(mem);
		void *mem2 = use_arena(original_size * 4, &arena);
		ptr_t *ptr2 = PTR_META(mem2);
		add_to_free_list(ptr2, &arena);

		void *new_mem = realloc_in_place(ptr, new_size, &arena);
		ptr_t *new_ptr = PTR_META(new_mem);
		ASSERT(new_mem == mem);
		ASSERT(ptr->total_size == original_size + extra_size_needed);
		size_t ptr_2_new_size = original_size * 4 - extra_size_needed;
		ptr_t *free_tail =
			arena.free_ptr_tails[SIZE_CLASS(ptr_2_new_size)];
		ASSERT(free_tail);
		ASSERT(free_tail->prev_in_arena == new_ptr);
		ASSERT(free_tail->total_size == ptr_2_new_size);
	}
	{ // Next ptr is free and is just as big as needed
		arena_t arena = {0};
		size_t original_size = MEM_OFFSET + MIN_ALLOC_SIZE;
		size_t new_size = original_size * 2;
		void *mem = use_arena(original_size, &arena);
		ptr_t *ptr = PTR_META(mem);
		void *mem2 = use_arena(original_size, &arena);
		ptr_t *ptr2 = PTR_META(mem2);
		add_to_free_list(ptr2, &arena);
		realloc_in_place(ptr, new_size, &arena);
		ASSERT(ptr->total_size == original_size * 2);
		ASSERT(!arena.free_ptr_tails[SIZE_CLASS(original_size)]);
	}
	{ // There is no next pointer
		arena_t arena = {0};
		size_t original_size = MEM_OFFSET + MIN_ALLOC_SIZE;
		size_t new_size = original_size * 2;
		void *mem = use_arena(original_size, &arena);
		ptr_t *ptr = PTR_META(mem);
		realloc_in_place(ptr, new_size, &arena);
		ASSERT(ptr->total_size == new_size);
		ASSERT(arena.offset == new_size);
	}
}

int main(void) {
	test_use_arena();
	test_add_to_free_list();
	test_use_free_list();
	test_remove_from_free_list();
	test_megre_neighbouring_ptrs();
	test_move_ptr();
	test_realloc_in_place();

	test_print_results();
	return 0;
}
