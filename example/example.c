/* Include the library. */
#include <mem_alloc.h>

int main(void) {
	/* Allocate memory.*/
	void *mem = mem_alloc(1024);

	/* Expand or shring the memory. */
	void *new_mem = mem_realloc(mem, 2048);
	if (new_mem)
		mem = new_mem;

	/* Deallocate the memory. */
	mem_free(mem);

	return 0;
}
