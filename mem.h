#pragma once
#include "types.h"


typedef struct mheader {

    struct mheader *next;
    struct mheader *prev;

    uint32_t size;
    int32_t  is_available;
} mheader_t;


#define SIZEOF_POINTER		8
#define MEM_ALIGN(x)		(((x) + SIZEOF_POINTER-1) & ~(SIZEOF_POINTER - 1))

#define MHEADER_TO_VOIDPTR(x) (void*)((uint8_t*)(x) + sizeof(mheader_t))
#define VOIDPTR_TO_MHEADER(x) (mheader_t *)((uint8_t*)(x) - sizeof(mheader_t))


typedef struct mem_pool {
    mheader_t *blocks_list;
    uint32_t free_mem_size;
    uint32_t used_mem_size;

}mem_pool_t;



void mem_init (mem_pool_t *mpool, void *start, void *stop);
void *mem_alloc(mem_pool_t *mpool, uint32_t size);
void mem_connect_nearby_free_blocks(mem_pool_t *mpool, mheader_t *block);

void mem_free(mem_pool_t *mpool, void *p);
bool_t mem_test_1(mem_pool_t *mpool, uint32_t (*rand)(uint32_t));

