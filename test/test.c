#include <test.h>
#include "mem_alloc_utils.h"

TEST_INIT;

int main(void) {
	{
		size_t size = 1024;
		void *data = mem_alloc(size);
		ASSERT(data);
		ptr_t *ptr = (ptr_t*)((unsigned char*)data - DATA_OFFSET);
		// printf("%p\n", ptr);
		ASSERT(ptr == (ptr_t*)get_arena()->buff);
		ASSERT(!ptr->is_mmap);
		ASSERT(ptr->is_valid == true);
		ASSERT(ptr->data == data);
		ASSERT(!ptr->next_free);
		ASSERT(!ptr->prev_free);
		ASSERT(ptr->total_size == DATA_OFFSET + ROUNDUP(size));
	}

	test_print_results();
	return 0;
}
