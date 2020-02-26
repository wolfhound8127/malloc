#pragma once
#include "types.h"

/* Структура заголовка блока памяти */
typedef struct mheader {

    struct mheader *next;
    struct mheader *prev;

    uint32_t size;
    int32_t  is_available;
} mheader_t;

/* Выравнивание указателей */
#define SIZEOF_POINTER		8
#define MEM_ALIGN(x)		(((x) + SIZEOF_POINTER-1) & ~(SIZEOF_POINTER - 1))


#define MHEADER_TO_VOIDPTR(x) (void*)((uint8_t*)(x) + sizeof(mheader_t))
#define VOIDPTR_TO_MHEADER(x) (mheader_t *)((uint8_t*)(x) - sizeof(mheader_t))


typedef struct mem_pool {
    mheader_t *blocks_list;
    uint32_t free_mem_size;
    uint32_t used_mem_size;
}mem_pool_t;


/* Инициализация пула памяти */
void mem_init (mem_pool_t *mpool, void *start, void *stop);

/* Выделение памяти */
void *mem_alloc(mem_pool_t *mpool, uint32_t size);

/* Освобождение памяти, выделенной под указатель */
void mem_free(mem_pool_t *mpool, void *p);


bool_t mem_test_1(mem_pool_t *mpool, uint32_t (*rand)(uint32_t));

