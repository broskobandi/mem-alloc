#include <test.h>
#include "mem_alloc_utils.h"

TEST_INIT;

int main(void) {
	{
		void *data = mem_alloc(sizeof(int));
		ASSERT(data);
		ptr_t *ptr = (ptr_t*)((unsigned char*)data - DATA_OFFSET);
		ASSERT(ptr);
		ASSERT(ptr->total_mem == DATA_OFFSET + ROUNDUP(sizeof(int)));
	}

	test_print_results();
	return 0;
}
