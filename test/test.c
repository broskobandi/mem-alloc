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
	printf("%lu\n", ptr->total_size);
	printf("%lu\n", SIZE);
}

int main(void) {
	test_use_arena();

	test_print_results();
	return 0;
}
