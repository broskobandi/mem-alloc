#include <test.h>
#include "mem_alloc_utils.h"

TEST_INIT;

int main(void) {
	static const size_t ARENA_ALLOC_SIZE = ARENA_SIZE / 32;
	static const size_t MMAP_ALLOC_SIZE = ARENA_SIZE * 2;
	// static const size_t ITER = 1000;
	{ // 
		void *mem1 = mem_alloc(ARENA_ALLOC_SIZE);
		ASSERT(mem1);

		void *mem2 = mem_alloc(MMAP_ALLOC_SIZE);
		ASSERT(mem2);
	}

	test_print_results();
	return 0;
}
