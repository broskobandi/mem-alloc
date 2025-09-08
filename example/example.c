/* Include the library. */
#include <mem_alloc.h>

int main(void) {

	/* Allocate new memory. */
	void *mem = mem_alloc(1024);

	/* Reallocate existing memory. */
	mem_realloc(mem, 2048);

	/* Deallocate memory. */
	mem_free(mem);

	return 0;
}
