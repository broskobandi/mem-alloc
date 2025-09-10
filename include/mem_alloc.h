#ifndef MEM_ALLOC_H
#define MEM_ALLOC_H

#include <stddef.h> /* For size_t */

void *mem_alloc(size_t size);
int mem_free(void *ptr);
void *mem_realloc(void *ptr, size_t size);

#endif
