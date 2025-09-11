# mem_alloc
## Memory allocator written in C
The goal of the project is to provide a highly performant allocator for 
moderately sized programs, such as simple game engines, that require frequent
allocations and deallocations. 
## Design decisions and future plans
Currently the library relies on some posix-specific headers, such as
unistd.h (for querying the page size) and sys/mman.h (for heap allocations).
The decision to use getpagesize() from unistd to query the page size was 
made so that it didn't have to be hardcoded in some assumed value. I plan
to make the inclusion of these header files conditional based on what platform 
the library is being compiled for. However, I might completely change any
calls to mmap and munmap to malloc and free respectively to improve
portability, but I wanted to avoid using the very mechanisms my library
sought to clone. Nonetheless, I am starting to realise that the most relevant
use case of my library is the performance gains provided by the static arena
and not the additional heap allocating feature. Based on my benchmark,
allocating and deallocating data using only the static buffer takes about
half as long as it does with malloc. Accessing the allocated data doesn't
show any meaningful difference. Because of this, I am thinking about demoting 
 the library to be purely a wrapper around malloc and not a competitor of it 
 that uses the same static buffer by default and then malloc as a fallback 
 allocator. With that design, the user could benefit from blazing fast 
 allocations and deallocations within the arena's boundaries and after that,
 the alloc and free calls would just call vanilla malloc and free internally.
## Features
### Performance and reliability
The library is designed around a static global arena which ensures that 
allocating, accessing, modifying, and deallocating memory is always fast 
and reliable. On top of this, the library does not use any loops apart from
the ones that may be executed internally by standard library functions, such
as memcpy(). The use of such functions is strictly limited to the bare
minimum in the implementation.
### Flexibility
The default arena size is 128KB. If your project needs more memory, recompile
the library with the following compile flag:
```bash
make EXTRA_CPPFLAGS=-DARENA_SIZE_MULTIPLIER=<N> &&
sudo make install
```
where <N> is the number with which you wish to multiply the default arena
size. 
### Performant thread safety
The static global arena instance is declared with the _Thread_local keyword
which ensures that no mutex locks are needed as each thread automatically 
receives a unique copy of the global arena. This reduces the risk of race
conditions and ensures good and reliable performance.
### Fallback to heap
When the arena runs out of memory, or when the memory to be allocated is
greater than the arena size, a new memory block is allocated with in the heap.
If you want to monitor when switching to heap allocations occurs in your 
application, I recommend recompiling the library with like this:
```bash
make debug &&
sudo make install
```
This will install the debug build which enables the printing of useful 
information during runtime, such as the size of the arena and the moment
when changing to heap allocations occurs.
## Installation
```bash
git clone https://github.com/broskobandi/mem_alloc.git &&
cd mem_alloc &&
make &&
sudo make install
```
Optionally, EXTRA_CPPFLAGS=-DARENA_SIZE_MULTIPLIER=<N> can be passed 
where N represents the number of times the arena size should be multiplied.
## Uninstallation
```bash
cd mem_alloc &&
sudo make uninstall
```
## Testing
```
cd mem_alloc &&
make clean &&
make test &&
make clean
```
## Documentation
Generating the documentation requires doxygen to be installed.
```bash
cd mem_alloc &&
make doc &&
```
Then open html/index.html to access the newly generated documentation.
## Usage
```c
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
```
Use the -L/use/local/lib -lmem_alloc compile flags when using the library 
in your application.
## Todo
- Allow for the reduction of the default arena size.
- Implement a free list for heap allocations as well.
- Improve portability: change mmap to malloc?
