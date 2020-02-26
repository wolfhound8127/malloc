
#include "mem.h"

#include <stdlib.h>
uint32_t rand_size(uint32_t max)
{
	return (uint32_t)rand() % max;
}

mem_pool_t mpool;
#define HEAP_SIZE 0x10000
unsigned char heap[HEAP_SIZE];

int main()
{
	void *heap_start = &heap[0];
	void *heap_end = &heap[HEAP_SIZE - 1];
	mem_init(&mpool, heap_start, heap_end);

	srand(1);
	
	bool_t ok = mem_test_1(&mpool, rand_size);

	while(1);
}
