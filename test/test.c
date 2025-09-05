#include <test.h>
#include "mem_alloc_utils.h"

TEST_INIT;

int main(void) {
	{
		void *data = mem_alloc(1024);
		ASSERT(data);
	}

	test_print_results();
	return 0;
}
