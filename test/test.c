#include <test.h>
#include "mem_alloc_utils.h"

TEST_INIT;

int main(void) {
	{ // mem_alloc, mem_free
		// mem_alloc
		size_t size = 1024;
		void *data = mem_alloc(size);
		ASSERT(data);
		ptr_t *ptr = (ptr_t*)((unsigned char*)data - DATA_OFFSET);
		arena_t *arena = get_arena();
		ASSERT(ptr == (ptr_t*)arena->buff);
		ASSERT(!ptr->is_mmap);
		ASSERT(ptr->is_valid == true);
		ASSERT(ptr->data == data);
		ASSERT(!ptr->next_free);
		ASSERT(!ptr->prev_free);
		size_t exp_size = DATA_OFFSET + ROUNDUP(size, MIN_ALLOC_SIZE);
		ASSERT(ptr->total_size == exp_size);
		ASSERT(ptr == arena->ptr_list_tail);

		// mem_free
		mem_free(data);
		ASSERT(!ptr->is_valid);
		ptr_t *free_tail = arena->free_ptr_tails[PTR_SIZE_CLASS(size)];
		ASSERT(free_tail == ptr);
		data = NULL;

		// alloc from free list
		void *data2 = mem_alloc(size);
		ASSERT(data2);
		ptr_t *ptr2 = (ptr_t*)((unsigned char*)data2 - DATA_OFFSET);
		ASSERT(ptr2 == ptr);
		ASSERT(ptr2->is_valid);
		ASSERT(ptr->is_valid); // this may be an issue functionality-
				       // wise?
		ASSERT(!arena->free_ptr_tails[PTR_SIZE_CLASS(size)]);
		free_tail = NULL;
		ASSERT(ptr2 == arena->ptr_list_tail);

		// mmap, munmap
		void *data3 = mem_alloc(ARENA_SIZE * 2);
		ASSERT(data3);
		ptr_t *ptr3 = (ptr_t*)((unsigned char*)data3 - DATA_OFFSET);
		ASSERT(ptr3->is_mmap == true);
		ASSERT(ptr2 == arena->ptr_list_tail);
		mem_free(data3);
		data3 = NULL;
		ASSERT(!arena->free_ptr_tails[PTR_SIZE_CLASS(size)]);

		// Update ptr list tail
		void *data4 = mem_alloc(size / 2);
		ASSERT(data4);
		ptr_t *ptr4 = (ptr_t*)((unsigned char*)data4 - DATA_OFFSET);
		ASSERT(ptr4 == arena->ptr_list_tail);
		ASSERT(ptr2 == arena->ptr_list_tail->prev_valid);
	}

	test_print_results();
	return 0;
}
