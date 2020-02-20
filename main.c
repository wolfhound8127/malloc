
#include "nvcom02t/nvcom02t.h"
#include "mem.h"


mem_pool_t mpool;

unsigned char heap[0x2000];

int main()
{

	mem_init(&mpool, &heap[0], &heap[0x2000-1]);
	void *p0 = mem_alloc(&mpool, 7);
	void *p1 = mem_alloc(&mpool, 16);
	void *p2 = mem_alloc(&mpool, 20);
	void *p3 = mem_alloc(&mpool, 24);
	void *p4 = mem_alloc(&mpool, 64);
	void *p5 = mem_alloc(&mpool, 17);
	void *p6 = mem_alloc(&mpool, 23);

	mem_free(&mpool, p2);
	mem_free(&mpool, p4);
	mem_free(&mpool, p3);

	while(1);
}
