# mem-alloc
Memory allocator written in C.

## Goal
The aim of the project is to provide a simple yet highly performant allocator for moderately sized projects, such as simple game engines, that require fast dynamic memory allocation for many small objects.

## Features
- By default, all allocations happen in an encapsulated thread-local static global buffer of size 128KB. This size can be adjusted by compiling the library with -DARENA_SIZE_CUSTOM=<custom_size> compiler flag. Thread locality is achieved by using the _Thread_local keyword which eliminates the need for any mutex locks which further aids performance.
- mmap() is used as a fallback allocation mechanism for sizes greater than ARENA_SIZE or when the arena is full.
- A unique free list is maintained for every possible allocation size in the arena that ensures fast allocations of sizes that have once been freed. 
- On freeing, neighbouring free pointers get merged automatically.
- No loops (apart from the ones that may be performed by standard library functions, such as memcpy(), but even these calls are reduced to the bare minimum).
- The automatic merge functionality increases the likelihood of reallocations in place which is also in support of good performance.

## Usage
Please refer to example/example.c for details about the usage.

## Todo
- Add free list for mmap allocations.
- Implement broader size classes.
- Make mmap fallback mechanism opt-out?
- Add documentation
