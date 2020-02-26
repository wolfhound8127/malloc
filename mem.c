#include "mem.h"


/*
 * Удаление блока из списка и обновление ссылок между соседними элементами.
 * 
 * return1: Удаление единственного блока возвращает nullptr.
 * return2: В остальных случаях возвращает предыдущий блок.
*/
mheader_t *mem_blocks_list_remove(mem_pool_t *mpool, mheader_t *block)
{
    if(!block)
        return nullptr;

    mheader_t *prev = nullptr;
    if(block != block->prev )
    {

        block->prev->next = block->next;
        block->next->prev = block->prev;

        prev = block->prev;

    }

    block->next = block->prev = nullptr;

    
    mpool->blocks_list = prev;

    return prev;
}

/*
 * Добавление нового блока в список и обновление ссылок между соседними элементами.
 * 
 * Если добавляемый блок единственный: (prev == nullptr)
 * ссылки блока устанавливаются на себя самого
*/
void mem_blocks_list_insert(mem_pool_t *mpool, mheader_t *prev, mheader_t *block)
{

    if(!block || prev == block )
        return;

    if(prev == nullptr)
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

/*
 * Поиск подходящего по размеру блока из списка
*/
mheader_t *mem_find_suitable_block(mem_pool_t *mpool,  uint32_t required_size)
{
    mheader_t *block = mpool->blocks_list;

    do
    {
        if(block->size >= required_size && block->is_available)
            return block;

        block = block->next;

    } while (block != mpool->blocks_list);
    

    return nullptr;
}

/* 
 * Обновление статистики используемой/свободной памяти
 * Положительное значение параметра size увеличивает количество доступной памяти,
 * отриацательное - уменьшает.
*/
static
inline  __attribute__ ((always_inline))
void mem_change_free_size_stat(mem_pool_t *mpool, int32_t size)
{
    mpool->free_mem_size += size;
    mpool->used_mem_size -= size;
}

/*
 * Начальная инициализация пула памяти
 * */
void mem_init (mem_pool_t *mpool, void *start, void *stop)
{
    /* Размер структуры заголовка должен быть равен выравниванию */
    BUILD_BUG_ON( sizeof(mheader_t) % SIZEOF_POINTER );

    mpool->blocks_list = nullptr;
    mpool->used_mem_size = 0;

    /* Выравнивание начала пула */
    void *aligned_start = (void*)MEM_ALIGN((uint32_t)start);


    /* Добавление первого свободного блока размером с весь пул */
    mheader_t *free_block = (mheader_t *)aligned_start ;
    free_block->size = (uint32_t)stop - (uint32_t)aligned_start  - sizeof(mheader_t);
    free_block->is_available = 1;
    mem_blocks_list_insert(mpool, mpool->blocks_list, free_block);

    
    mpool->free_mem_size = free_block->size;
    /* Заголовок свободного блока считается используемой памятью*/
    mpool->used_mem_size = sizeof(mheader_t);


}

/* 
 * Выделение памяти
 */
void *mem_alloc(mem_pool_t *mpool, uint32_t size)
{

    uint32_t required_size = MEM_ALIGN (size);

    mheader_t *new_used_block = mem_find_suitable_block(mpool, required_size);
    if(!new_used_block) return nullptr;

    uint32_t left_free_space = new_used_block->size - required_size;

    new_used_block->size = required_size;
    mem_change_free_size_stat(mpool, -required_size);
    new_used_block->is_available = 0;


    if(left_free_space >= sizeof(mheader_t))
    {
        mheader_t *next = (mheader_t *)((uint8_t*)new_used_block + sizeof(mheader_t) + new_used_block->size);
        next->size = left_free_space - sizeof(mheader_t);
        mem_change_free_size_stat(mpool, -sizeof(mheader_t));
        next->is_available = 1;
        mem_blocks_list_insert(mpool, new_used_block, next);
    }


    return MHEADER_TO_VOIDPTR(new_used_block);

}

/*
 * Освобождение памяти, выделенной под указатель
 * Если рядом с освобождаемым блоком есть другие свободные блоки,
 * то выполняется дефрагментация:
 * последовательность свободных блоков объединятся в один блок.
 * */
void mem_free(mem_pool_t *mpool, void *p)
{

    mheader_t *block = VOIDPTR_TO_MHEADER(p);
    if(block->is_available)
    	return;

    block->is_available = 1;

    mem_change_free_size_stat(mpool, block->size);

    mem_connect_nearby_free_blocks(mpool, block);
}

/* Дефрагментация: последовательность свободных блоков рядом объединятся в один блок.*/
void mem_connect_nearby_free_blocks(mem_pool_t *mpool, mheader_t *block)
{
    if(!block || block->is_available == 0)
        return;

    /* Поиск следующих в списке свободных блоков
     * и объединение с текущим*/
    mheader_t *next = block->next;
    while(next->is_available && next != next->next)
    {
        block->size += next->size + sizeof(mheader_t);//!!!!
        mem_change_free_size_stat(mpool, sizeof(mheader_t));
        block = mem_blocks_list_remove(mpool, next);
        next = block->next;
    }

    /* Поиск предшествующих в списке свободных блоков и объединение */
    mheader_t *prev = block->prev;
    while (prev->is_available && prev != prev->prev)
    {
        prev->size += prev->next->size +  sizeof(mheader_t);//!!!!
        mem_change_free_size_stat(mpool, sizeof(mheader_t));
        block = mem_blocks_list_remove(mpool, prev->next);
        prev = block->prev;
    }

}

/* Принудительно обеспечить пул свободным блоком указанного размера */
bool_t mem_force_provide_suitable_block(mem_pool_t *mpool, uint32_t size)
{
    mheader_t *block = mpool->blocks_list;
    do
    {
        if(block->size >= size)
        {
            if(!block->is_available)
            {
                mem_free(mpool, MHEADER_TO_VOIDPTR(block));
            }

            return true;
        }

        block = block->next;

    } while (block != mpool->blocks_list);
    

    return false;
}

/* Тестирование пула.
 * Выделяется память до полного заполнения пула.
 * Размер размещаемого блока задается с помощью функции rand
 * Функция rand должна возвращать положительное число в пределах [0, mpool->free_mem_size)
 * Если случайное число больше половины свободной памяти в пуле,
 * то вызывается функция mem_force_provide_suitable_block
 * Если случайное число больше половины свободной памяти в пуле,
 * то выделяется блок памяти указанного размера
 * 
 * Как только весь пул будет заполнен, выполняется полная очитка пула.
 * Если счетчики статистики совпадают с первоначальными значениями, 
 * то считается, что тест пройден успешно. 
 */
bool_t mem_test_1(mem_pool_t *mpool, uint32_t (*rand)(uint32_t))
{
    uint32_t free_space = mpool->free_mem_size;
	uint32_t used_space = mpool->used_mem_size;

    uint32_t random_number;
    bool_t ok = true;
	do
	{
		random_number = rand(mpool->free_mem_size);
        if(random_number > mpool->free_mem_size>>1)
        {
            ok = mem_force_provide_suitable_block(mpool, random_number);
        }
        else
        {
            ok = (mem_alloc(mpool, random_number) != nullptr);
        }
		
	} while (ok);

    mheader_t *block;
    do
    {
        block = mpool->blocks_list;
        while (block->is_available == 1)  
        {
            if(block->prev == block)
                break;
                
            block = block->next;
        }
        
        mem_free(mpool, MHEADER_TO_VOIDPTR(block));

    } while (block->prev != block);


    return (free_space == mpool->free_mem_size) && (used_space == mpool->used_mem_size);

}

