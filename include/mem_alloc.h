#ifndef MEM_ALLOC_H
#define MEM_ALLOC_H

#include <stddef.h> /* For size_t */

void *mem_alloc(size_t size);
void mem_free(void *mem);
void *mem_realloc(void *mem, size_t size);

#endif
