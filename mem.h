#pragma once
#include "types.h"


typedef struct mheader {

    struct mheader *next;
    struct mheader *prev;

    uint32_t size;
    int32_t  is_available;
} mheader_t;
#define SIZEOF_MHEADER sizeof(mheader_t)

#if ((SIZEOF_MHEADER % SIZEOF_POINTER) != 0)
#pragma message("Size of mheader_t must be multiply SIZEOF_POINTER")
#endif

typedef struct mem_pool {
    mheader_t *blocks_list;

    uint32_t free_mem_size;
    uint32_t used_mem_size;

}mem_pool_t;


#define SIZEOF_POINTER		8//sizeof(void*)
#define MEM_ALIGN(x)		(((x) + SIZEOF_POINTER-1) & ~(SIZEOF_POINTER - 1))

void mem_init (mem_pool_t *mpool, void *start, void *stop);
void mem_join_free(mem_pool_t *mpool, mheader_t *block);

void mem_free(mem_pool_t *mpool, void *p);
