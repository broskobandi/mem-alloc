#include <test.h>
#include "mem_alloc_utils.h"

TEST_INIT;

int main(void) {
	static const size_t ARENA_ALLOC_SIZE = ARENA_SIZE / 32;
	static const size_t MMA_ALLOC_SIZE = ARENA_SIZE * 2;
	{
		void *mem = mem_alloc(ARENA_ALLOC_SIZE);
		ASSERT(mem);
		void *mem2 = mem_alloc(MMA_ALLOC_SIZE);
		ASSERT(mem2);
	}

	test_print_results();
	return 0;
}
