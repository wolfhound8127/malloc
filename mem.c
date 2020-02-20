#include "mem.h"

void mem_init (mem_pool_t *mpool, void *start, void *stop)
{
    mpool->blocks_list = nullptr;
    mpool->free_mem_size = 0;
    mpool->used_mem_size = 0;

    if( sizeof(mheader_t) % SIZEOF_POINTER != 0)
        return;

    void *aligned_start = (void*)MEM_ALIGN((uint32_t)start);
    mheader_t *free_block = (mheader_t *)aligned_start ;
    free_block->size = (uint32_t)stop - (uint32_t)aligned_start  - sizeof(mheader_t);
    free_block->is_available = 1;

    mem_blocks_list_insert(mpool, mpool->blocks_list, free_block);
    mpool->free_mem_size = free_block->size;

}
static
inline  __attribute__ ((always_inline))
mheader_t* mem_next_mheader(mheader_t *h)
{
    return h->next;
}

static
inline  __attribute__ ((always_inline))
mheader_t* mem_prev_mheader(mheader_t *h)
{
    return h->prev;
}

mheader_t *mem_blocks_list_remove(mem_pool_t *mpool, mheader_t *block)
{
    mheader_t *prev;
    if(block->next != block->prev )
    {

        prev = block->prev;


        block->prev->next = block->next;
        block->next->prev = block->prev;
        
        block->next = block->prev = nullptr;

    }
    else
    {
        block->next = block->prev = nullptr;
        prev = nullptr;
        

    }
    mpool->blocks_list = prev;

    return prev;
}

void mem_blocks_list_insert(mem_pool_t *mpool, mheader_t *prev, mheader_t *block)
{

    if(prev == nullptr || block == prev)
    {
        block->next = block;
        block->prev = block;
    }
    else
    {
        block->next = prev->next;
        prev->next = block;

        block->prev = prev;
        block->next->prev = block;
    }
    
    mpool->blocks_list = block;
}

mheader_t *mem_find_suitable_block(mem_pool_t *mpool,  uint32_t required_size)
{
    mheader_t *block = mpool->blocks_list;

    while (block->is_available == 0 || block->size < required_size)
    {

        block = mem_next_mheader(block);

        if(block == mpool->blocks_list)
            return nullptr;
    }

    return block;
}

void *mem_alloc(mem_pool_t *mpool, uint32_t size)
{

    uint32_t required_size = MEM_ALIGN (size);

    mheader_t *suitable_block = mem_find_suitable_block(mpool, required_size);

    uint32_t left_free_space = suitable_block->size - required_size;

    mheader_t *new_used_block = suitable_block;
    new_used_block->size = required_size;
    new_used_block->is_available = 0;


    
    if(left_free_space >= sizeof(mheader_t))
    {
        mheader_t *new_free_block = (uint8_t*)new_used_block + sizeof(mheader_t) + new_used_block->size;
        new_free_block->size = left_free_space - sizeof(mheader_t);
        new_free_block->is_available = 1;
        mem_blocks_list_insert(mpool, new_used_block, new_free_block);
    }

    mpool->free_mem_size -= required_size;
    mpool->used_mem_size += required_size;

    return (void*)((uint8_t*)new_used_block + sizeof(mheader_t));

}

void mem_free(mem_pool_t *mpool, void *p)
{
    mheader_t *block = (uint8_t*)p - sizeof(mheader_t);

    block->is_available = 1;

    mpool->free_mem_size += block->size;
    mpool->used_mem_size -= block->size;

    mem_join_free(mpool, block);
}

void mem_join_free(mem_pool_t *mpool, mheader_t *block)
{
    if(!block || block->is_available == 0)
        return nullptr;

    mheader_t *next = mem_next_mheader(block);
    while(next->is_available)
    {
        block->size += next->size;
        mpool->free_mem_size += sizeof(mheader_t);
        mpool->used_mem_size -= sizeof(mheader_t);
        block = mem_blocks_list_remove(mpool, next);
        next = mem_next_mheader(block);
    }

    mheader_t *prev = mem_prev_mheader(block);
    while (prev->is_available)
    {
        prev->size += mem_next_mheader(prev)->size;
        mpool->free_mem_size += sizeof(mheader_t);
        mpool->used_mem_size -= sizeof(mheader_t);
        block = mem_blocks_list_remove(mpool, mem_next_mheader(prev));
        prev = mem_prev_mheader(block);
    }

}

