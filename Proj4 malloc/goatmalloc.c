#include <stdio.h>
#include <stddef.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "goatmalloc.h"


/////////////
// GLOBALS //
/////////////

void* _arena_start;
int statusno;
int max_mem;

/////////////
// STRUCTS //
/////////////


// init //
int init(size_t size) {
	int arena_size = 0;
	//int found_free = 0;
	// initializes memory allocator
	printf("Initializing arena:\n");
	printf("...requested size %zu bytes\n", size);
	printf("...pagesize is %zu bytes\n", size);
	int fd = open("/dev/zero", O_RDWR);
	if (size > 10000000) return ERR_BAD_ARGUMENTS;

	printf("...adjusting size with page boundaries\n");
	if(size < getpagesize()) {
		size = getpagesize();
	} else if (size > getpagesize()) {
		size += getpagesize() - (size % getpagesize());
	}
	printf("...adjusted size is %zu bytes\n", size);

	printf("...mapping arena with mmap()\n");

	node_t *chunk;

	if(_arena_start == NULL) {

		_arena_start = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
		printf("...arena starts at %p\n", _arena_start);
		chunk = _arena_start;

		arena_size += size;
		chunk->size = size - sizeof(node_t);
		chunk->fwd = NULL;

	} else {

		printf("...arena starts at %p\n", _arena_start);
		node_t *tmp;
		tmp = _arena_start;
		arena_size += tmp->size;

		while(tmp->fwd != NULL) {
			tmp = tmp->fwd;
			arena_size += tmp->size;

		}

		chunk = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);

		tmp->fwd = chunk;

		chunk->size = size - sizeof(node_t);
		chunk->bwd = tmp;
		chunk->fwd = NULL;

		arena_size += chunk->size;
		
	}

	printf("...arena ends at %p\n", chunk+(size/sizeof(node_t)));

	printf("...initializing header for initial free chunk\n");
	printf("...header size is %zu bytes\n", sizeof(chunk));
	chunk->is_free = 1;
	
	max_mem = arena_size;
	return arena_size;

}


int destroy() {
	if(_arena_start == NULL) return ERR_UNINITIALIZED;
	node_t *chunk = _arena_start;
	printf("Destroying Arena:\n");
	printf("...unmapping arena with munmap()\n");
	munmap((_arena_start+chunk->size)-max_mem, max_mem);
	_arena_start = NULL;

	return 0;

}


void* walloc(size_t size) {
	int found_free = 0;
	printf("Allocating memory:\n");

	if (_arena_start == NULL) {
		statusno = ERR_UNINITIALIZED;
		return NULL;

	}

	node_t *tmp;

	printf("...looking for free chunk of >= %zu bytes\n", size);

	size += sizeof(node_t);

	tmp = _arena_start;

	printf("...free chunk->fwd currently points to %p\n", tmp->fwd);
	printf("...free chunk->bwd currently points to %p\n", tmp->bwd);

	if(tmp->is_free == 1 && tmp->size >= size-sizeof(node_t)){
		printf("...found free chunk of %zu bytes with header at %p\n", tmp->size, tmp);
		printf("...updating chunk header at %p\n", tmp);
		printf("...being careful with my pointer arithmetic and void poitner casting\n");

		if (size < tmp->size - sizeof(node_t) - 1) {
			printf("...checking if splitting is required\n");
			int remaining_space = tmp->size-(size-sizeof(node_t))-(sizeof(node_t));
			node_t *new_node = (void*)tmp + sizeof(node_t) + size-sizeof(node_t);

			new_node->size = remaining_space;
			new_node->is_free = 1;
			new_node->fwd = tmp->fwd;
			new_node->bwd = tmp;

			tmp->size = size - sizeof(node_t);
			tmp->fwd = new_node;
		}


		found_free = 1;

	} else {

		while(tmp->fwd != NULL) {
			tmp = tmp->fwd;

			printf("...checking if splitting is required\n");
			if(tmp->is_free == 1 && tmp->size >= size-sizeof(node_t)) {

				if (size < tmp->size - sizeof(node_t) - 1) {
					int remaining_space = tmp->size-(size-sizeof(node_t))-(sizeof(node_t));
					node_t *new_node = (void*)tmp + sizeof(node_t) + sizeof(node_t);

					new_node->size = remaining_space;
					new_node->is_free = 1;
					new_node->fwd = tmp->fwd;
					new_node->bwd = tmp;

					tmp->size = size-sizeof(node_t);
					tmp->fwd = new_node;
				}

				found_free = 1;
				break;
			}

			else {printf("...splitting not required\n");}
		}
	}

	if (!found_free) {
		statusno = ERR_OUT_OF_MEMORY;
		return NULL;

	}

	tmp->is_free = 0;

	printf("...allocation starts at %p\n,", tmp);
	

	return tmp + 1;
}


void wfree(void *ptr) {
	// free  up existing memory chunks
	printf("Freeing allocated memory:\n");
	printf("...supplied pointer %p\n", ptr);
	printf("...being careful with my pointer arithmetic and void pointer casting\n");
	
	node_t *tmp = ptr - sizeof(node_t);

	printf("...accessing chunk header at %p\n", tmp);
	printf("...chunk of size %zu\n", tmp->size);

	munmap(tmp + sizeof(node_t), tmp->size);

	tmp->is_free = 1;

	node_t *prev = tmp;
	node_t *prev2 = tmp;
	printf("...checking if coalescing is needed\n");

	while (prev->fwd != NULL) {
		prev = prev->fwd;

		if (prev->is_free == 1) {
			tmp->size += prev->size + sizeof(node_t);
			tmp->fwd = prev->fwd;
			munmap(prev, prev->size);

		} else {
			break;
		}


	}

	while (prev2->bwd != NULL) {
		prev2 = prev2->bwd;

		if(prev2->is_free == 1) {

			prev2->size += tmp->size + sizeof(node_t);
			prev2->fwd = tmp->fwd;
			if(prev->fwd != NULL)
				prev2->fwd->bwd = prev2;

			munmap(tmp, tmp->size);

			tmp = prev2;

		} else {
			break;
		}
	}

	printf("...coalescing not needed.\n");

}

int main() {
	init(getpagesize());
	
	int* int1 = walloc(sizeof(int)*10);
	printf("%p", int1);

	init(getpagesize());
	int* int2 = walloc(sizeof(int)*10);
	print("%p", int1);
	print("%p", int2);
}
