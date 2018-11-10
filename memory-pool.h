/*
memory-pool.h - public domain - github.com/cofinite

In exactly one source file, put:
    #define MEMORY_POOL_IMPLEMENTATION
    #include "memory-pool.h"
    
Other source or header files should have just:
    #include "memory-pool.h"


The purpose of this library is to provide fast, dynamic allocation of fixed-
size objects in portable C89 and to do so in a generic, type-safe, and 
dependency-free way.

How to use:
    
    // creating a memory pool for objects of type `MyType`
    MemPool(MyType) pool = mpInit(&pool);
    
    // allocating an object and getting its handle
    size_t handle = mpAlloc(&pool);
    
    // accessing the allocated object via its handle
    mpAt(&pool, handle).foo = whatever;
    mpAt(&pool, handle).bar = something;
    
    // deallocating the object via its handle
    mpFree(&pool, handle);
    
    // releasing all memory associated with the memory pool
    mpFreePool(&pool);
    
Notes:
    
    The capacity of a pool is 0 after initialization. A pool will 
    automatically resize if an object is requested but none are left. 
    `mpCapacity` returns the current capacity of a pool.
    
    If the number of objects you are about to allocate is known prior, you may 
    call `mpGrowPool` to add to the pool's capacity manually. `mpGrowPool` 
    returns 0 on success and -1 in an out-of-memory situation.
    
    `mpAlloc` returns a valid object handle of type `size_t` on success and 
    `MP_INVALID_HANDLE` in an out-of-memory situation. Addresses of allocated 
    objects are not stable and may change when the pool resizes, but their 
    handles will remain valid until they are freed via `mpFree` or `mpFreePool`.
    
    In an out-of-memory situation, `mpGrowPool` and `mpAlloc` leave the pool in 
    a usable state and do not alter it.
    
    An object is accessed by passing its handle to the `mpAt` macro, which 
    expands out to an lvalue of the object.
    
    `mpAt` and `mpFree` do not perform bounds checking, nor do they check that 
    the handle is valid. Call them only with valid handles, lest you segfault 
    or corrupt the pool.
    
    `mpFree` makes an object available for `mpAlloc` to allocate again, but 
    will never decrease the capacity of a pool. To free memory associated 
    with a pool, you must free the entire pool using `mpFreePool`.
    
    After a call to `mpFreePool`, the capacity of a pool is 0 and all 
    previously allocated handles are invalid. The pool can then be used again 
    to allocate new handles.
    
    Identifiers defined by this library suffixed by an underscore (`_`) are for 
    internal use only. Your code should not contain any of them.
    
    If you need to refer to the same type of `MemPool` several times, you 
    should `typedef` it first:
        
        typedef MemPool(int) IntPool;


LICENSE

See end of file for license information.

*/

#ifndef MEMORY_POOL_H_INCLUDED
#define MEMORY_POOL_H_INCLUDED

#include <stddef.h>

struct MemPool_ {
    union {
        size_t  next;
    } *pBlocks;
    size_t  capacity;
    size_t  hFreeArray;
    size_t  hFreeList;
    size_t  blockSize;
};

#define MemPool(type)       \
union {                     \
    struct MemPool_ pool_;  \
    union {                 \
        size_t  next;       \
        type    value;      \
    } *pBlocks_;            \
}

#define mpInit(pPool)       {{NULL, 0, 0, -1, sizeof(*(pPool)->pBlocks_)}}
#define mpAt(pPool, handle) ((pPool)->pBlocks_[handle].value)
#define mpCapacity(pPool)   ((const size_t)(pPool)->pool_.capacity)

#define mpGrowPool(pPool, num)   mpGrowPool_(&(pPool)->pool_, (num))
#define mpFreePool(pPool)        mpFreePool_(&(pPool)->pool_)
#define mpAlloc(pPool)           mpAlloc_(&(pPool)->pool_)
#define mpFree(pPool, handle)    mpFree_(&(pPool)->pool_, (handle))

int     mpGrowPool_ (struct MemPool_* this, size_t num);
void    mpFreePool_ (struct MemPool_* this);
size_t  mpAlloc_    (struct MemPool_* this);
void    mpFree_     (struct MemPool_* this, size_t handle);

#define MP_INVALID_HANDLE ((size_t)(-1))

#endif /* MEMORY_POOL_H_INCLUDED */



#ifdef MEMORY_POOL_IMPLEMENTATION

#include <stdlib.h>

static int mpResize_(struct MemPool_* this, size_t capacity)
{
    void* temp = realloc(this->pBlocks, capacity * this->blockSize);
    if (temp == NULL) {
        return -1;
    }
    this->pBlocks = temp;
    this->capacity = capacity;
    return 0;
}

static size_t* mpNext_(struct MemPool_* this, size_t handle)
{
    return (size_t*)((char*)this->pBlocks + handle * this->blockSize);
}

int mpGrowPool_(struct MemPool_* this, size_t num)
{
    size_t newCapacity = this->capacity + num;
    if (newCapacity < this->capacity) {
        return -1;
    }
    return mpResize_(this, this->capacity + num);
}

void mpFreePool_(struct MemPool_* this)
{
    if (this->pBlocks != NULL) {
        free(this->pBlocks);
        this->pBlocks = NULL;
    }
    this->capacity = 0;
    this->hFreeArray = 0;
    this->hFreeList = MP_INVALID_HANDLE;
}

size_t mpAlloc_(struct MemPool_* this)
{
    size_t handle = this->hFreeList;
    if (handle != MP_INVALID_HANDLE) {
        this->hFreeList = *mpNext_(this, handle);
        return handle;
    }
    if (this->hFreeArray >= this->capacity) {
        size_t newCapacity = this->capacity * 3 / 2;
        if (newCapacity < this->capacity) {
            return MP_INVALID_HANDLE;
        }
        if (newCapacity == this->capacity) {
            newCapacity += 1;
        }
        if (mpResize_(this, newCapacity) != 0) {
            return MP_INVALID_HANDLE;
        }
    }
    handle = this->hFreeArray;
    this->hFreeArray += 1;
    return handle;
}

void mpFree_(struct MemPool_* this, size_t handle)
{
    *mpNext_(this, handle) = this->hFreeList;
    this->hFreeList = handle;
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
