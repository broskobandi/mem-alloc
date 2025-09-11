/*
MIT License

Copyright (c) 2025 broskobandi

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/** \file include/mem_alloc.h
 * \brief Public header file for the mem_alloc library. 
 * \details This file contains the forward declarations of public funcions 
 * for the mem_alloc library. */

#ifndef MEM_ALLOC_H
#define MEM_ALLOC_H

#include <stddef.h> /* For size_t */

/******************************************************************************
 * Public function forward declarations
 *****************************************************************************/

/** Allocates memory of 'size' bytes in an internal static buffer or in the 
 * heap if the buffer is full.
 * \param size The number of bytes to allocate.
 * \return A pointer to the allocated memory or NULL on failure. */
void *mem_alloc(size_t size);

/** Deallocates memory pointed to by 'ptr'.
 * \param ptr A pointer to the memory to be freed.
 * \return 0 if heap memory was successfully unmapped,
 * 1 if the memory was at the end of the internal buffer and therefore 
 * only the offset was updated, 2 if the memory was in the middle of the
 * buffer and is now added to the free list, -1 on failure. */
int mem_free(void *ptr);

/** Reallocates the allocated memory pointed to by 'ptr' 
 * by either shrinking it or expanding it to be 'size' number of bytes. 
 * \param ptr The pointer to the memory to be resized.
 * \param size The size of the new allocation in bytes. 
 * \return A pointer to the new memory location (which may be 
 * the same as the pointer passed to it in case in-place reallocation was
 * possible) or NULL on failure. */
void *mem_realloc(void *ptr, size_t size);

#endif
