/*
memory-pool.h - public domain - github.com/cofinite

In exactly one source file, put:
    #define MEMORY_POOL_IMPLEMENTATION
    #include "memory-pool.h"
    
Other source or header files should have just:
    #include "memory-pool.h"


The purpose of this library is to provide fast, dynamic allocation of fixed-
size objects in portable C89 and to do so in a generic, type-safe[1], and
dependency-free way.

How to use:

    To create a memory pool for objects of type `T`, declare a variable with the
    type `MemPool(T)` and initialize it using the macro `mpInit`:

        MemPool(T) pool = mpInit(&pool);
        
    Then, you may call `mpGrowPool` to increase the initial capacity of the
    pool[2], or simply begin calling `mpAlloc` to start allocating objects:
        
        mpGrowPool(&pool, 100); // optional
        size_t handle = mpAlloc(&pool);
        
    `mpAlloc` returns a handle into the memory pool[3]. To access an object via
    its handle, use the `mpAt` macro[4]:
        
        mpAt(&pool, handle).foo = 1;
        mpAt(&pool, handle).bar = mpAt(&pool, handle).foo + 1;
        
    Once you are done using an object, call `mpFree` with the object's
    handle[5]:
        
        mpFree(&pool, handle);
        
    Once you are done using the memory pool, call `mpFreePool` to release all
    memory associated with it:
        
        mpFreePool(&pool);
        
    After this, all previously allocated handles into the pool will be invalid.
    The same pool may be used again via `mpGrowPool` and `mpAlloc` to allocate
    new handles.

Notes:
    
    [1] Accessed objects have the correct type.
    
    [2] `mpGrowPool` returns 0 on success and -1 in an out-of-memory
    situation.
    
    [3] `mpAlloc` returns a valid handle of type `size_t` on success and
    `MP_INVALID_HANDLE` in an out-of-memory situation. Addresses of allocated
    objects are not stable and may change when the pool resizes, but handles
    will remain valid until they are freed via `mpFree` or `mpFreePool`.
    
    [4] `mpAt` does not perform bounds checking, nor does it make sure that the
    handle is valid. Call it only with valid handles, lest you segfault or
    corrupt the pool.
    
    [5] `mpFree` makes an object available for `mpAlloc` to allocate again, but
    does not actually decrea
    
    If you need to refer to the same type of `MemPool` several times, you should
    `typedef` it first:
        
        typedef MemPool(int) IntPool;


LICENSE

See end of file for license information.

*/

#ifndef MEMORY_POOL_H_INCLUDED
#define MEMORY_POOL_H_INCLUDED

#include <stddef.h>

typedef struct MemPoolInfo {
    union {
        size_t  next;
    } *pBlocks;
    size_t  capacity;
    size_t  hFreeArray;
    size_t  hFreeList;
    size_t  blockSize;
} MemPoolInfo;

#define MemPool(type)   \
union {                 \
    MemPoolInfo info;   \
    union {             \
        size_t  next;   \
        type    value;  \
    } *pBlocks;         \
}

#define mpInit(pMemPool)        {{NULL, 0, 0, -1, sizeof(*(pMemPool)->pBlocks)}}
#define mpAt(pMemPool, handle)  ((pMemPool)->pBlocks[handle].value)
#define mpCapacity(pMemPool)    ((const size_t)(pMemPool)->info.capacity)

#define mpGrowPool(pMemPool, num)   mpGrowPoolEx(&(pMemPool)->info, (num))
#define mpFreePool(pMemPool)        mpFreePoolEx(&(pMemPool)->info)
#define mpAlloc(pMemPool)           mpAllocEx(&(pMemPool)->info)
#define mpFree(pMemPool, handle)    mpFreeEx(&(pMemPool)->info, (handle))

int     mpGrowPoolEx    (MemPoolInfo* this, size_t num);
void    mpFreePoolEx    (MemPoolInfo* this);
size_t  mpAllocEx       (MemPoolInfo* this);
void    mpFreeEx        (MemPoolInfo* this, size_t handle);

#define MP_INVALID_HANDLE ((size_t)(-1))

#endif /* MEMORY_POOL_H_INCLUDED */



#ifdef MEMORY_POOL_IMPLEMENTATION

#include <stdlib.h>

#define _MP_GROWTH_FACTOR_NUMERATOR     3
#define _MP_GROWTH_FACTOR_DENOMINATOR   2

static int _mpResize(MemPoolInfo* this, size_t capacity)
{
    void* temp = realloc(this->pBlocks, capacity * this->blockSize);
    if (temp == NULL) {
        return -1;
    }
    this->pBlocks = temp;
    this->capacity = capacity;
    return 0;
}

static size_t* _mpNext(MemPoolInfo* this, size_t handle)
{
    return (size_t*)((char*)this->pBlocks + handle * this->blockSize);
}

int mpGrowPoolEx(MemPoolInfo* this, size_t num)
{
    size_t newCapacity = this->capacity + num;
    if (newCapacity < this->capacity) {
        return -1;
    }
    return _mpResize(this, this->capacity + num);
}

void mpFreePoolEx(MemPoolInfo* this)
{
    if (this->pBlocks != NULL) {
        free(this->pBlocks);
        this->pBlocks = NULL;
    }
    this->capacity = 0;
    this->hFreeArray = 0;
    this->hFreeList = MP_INVALID_HANDLE;
}

size_t mpAllocEx(MemPoolInfo* this)
{
    size_t handle = this->hFreeList;
    if (handle != MP_INVALID_HANDLE) {
        this->hFreeList = *_mpNext(this, handle);
        return handle;
    }
    if (this->hFreeArray >= this->capacity) {
        size_t newCapacity = this->capacity *
            _MP_GROWTH_FACTOR_NUMERATOR /
            _MP_GROWTH_FACTOR_DENOMINATOR;
        if (newCapacity < this->capacity) {
            return MP_INVALID_HANDLE;
        }
        if (newCapacity == this->capacity) {
            newCapacity += 1;
        }
        if (_mpResize(this, newCapacity) != 0) {
            return MP_INVALID_HANDLE;
        }
    }
    handle = this->hFreeArray;
    this->hFreeArray += 1;
    return handle;
}

void mpFreeEx(MemPoolInfo* this, size_t handle)
{
    if (handle != MP_INVALID_HANDLE) {
        *_mpNext(this, handle) = this->hFreeList;
        this->hFreeList = handle;
    }
}

#endif /* MEMORY_POOL_IMPLEMENTATION */

/*

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>

*/
