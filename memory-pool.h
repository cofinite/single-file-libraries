/*
memory-pool.h - public domain - github.com/cofinite

In exactly one source file, put:
    #define MEMORY_POOL_IMPLEMENTATION
    #include "memory-pool.h"
    
Other source or header files should have just:
    #include "memory-pool.h"


The purpose of this library is to provide fast, dynamic allocation of fixed-
size objects in portable C89 and to do so in a generic and dependency-free
way.

To create a memory pool for objects of type `T`, declare a variable with the
type `MemPool(T)` and initialize it using the macro `mpInit`:

    MemPool(T) pool = mpInit(&pool);
    
Then, you may call `mpGrowPool` to increase the initial capacity of the pool
before allocating any objects, or simply begin calling `mpAlloc` to start
allocating objects:
    
    mpGrowPool(&pool, 100);
    size_t handle = mpAlloc(&pool);
    
`mpAlloc` returns `size_t` handles into the memory pool, which are valid as
long as they are not freed via `mpFree` or the pool destroyed via
`mpDestroyPool`. To access an object via its handle, use the `mpAt` macro:
    
    T foo;
    foo.x = 1;
    foo.y = 2;
    foo.z = 3;
    
    mpAt(&pool, handle) = foo;
    
Once you are done with an object, call `mpFree` with the object's handle:
    
    mpFree(&pool, handle);
    
This will not free any memory, but it will make the object available for
`mpAlloc` to allocate again.

Once you are done using the memory pool, call `mpDestroyPool` to release all
memory associated with it:
    
    mpDestroyPool(&pool);
    
After this, all previously allocated handles into the pool will be invalid.
The same pool may be used again via `mpGrowPool` or `mpAlloc` to allocate new
handles.


LICENSE

See end of file for license information.

*/

#ifndef MEMORY_POOL_H_INCLUDED
#define MEMORY_POOL_H_INCLUDED

#include <stddef.h>

#define MemPool(type)   \
struct {                \
    union {             \
        type    value;  \
        size_t  next;   \
    } *pValues;         \
    size_t  capacity;   \
    size_t  hFreeArray; \
    size_t  hFreeList;  \
    size_t  valueSize;  \
}

#define mpInit(pMemPool)        {NULL, 0, 0, -1, sizeof(*(pMemPool)->pValues)}
#define mpAt(pMemPool, handle)  ((pMemPool)->pValues[handle].value)

int     mpGrowPool      (void* pMemPool, size_t num);
void    mpDestroyPool   (void* pMemPool);
size_t  mpAlloc         (void* pMemPool);
void    mpFree          (void* pMemPool, size_t handle);
size_t  mpCapacity      (void* pMemPool);

#define MP_INVALID_HANDLE ((size_t)(-1))

#endif /* MEMORY_POOL_H_INCLUDED */



#ifdef MEMORY_POOL_IMPLEMENTATION

#include <stdlib.h>

#define _MP_GROWTH_FACTOR_NUMERATOR     3
#define _MP_GROWTH_FACTOR_DENOMINATOR   2
#define _MP_DEFAULT_INITIAL_CAPACITY    16

typedef MemPool(size_t) _MpDummy;

static int _mpResize(_MpDummy* this, size_t capacity)
{
    void* temp = realloc(this->pValues, capacity * this->valueSize);
    if (temp == NULL) {
        return -1;
    }
    this->pValues = temp;
    this->capacity = capacity;
    return 0;
}

int mpGrowPool(void* pMemPool, size_t num)
{
    _MpDummy* this = pMemPool;
    return _mpResize(this, this->capacity + num);
}

void mpDestroyPool(void* pMemPool)
{
    _MpDummy* this = pMemPool;
    
    if (this->pValues != NULL) {
        free(this->pValues);
    }
    this->pValues = NULL;
    this->capacity = 0;
    this->hFreeArray = 0;
    this->hFreeList = MP_INVALID_HANDLE;
}

size_t mpAlloc(void* pMemPool)
{
    _MpDummy* this = pMemPool;
    size_t handle = this->hFreeList;
    if (handle != MP_INVALID_HANDLE) {
        this->hFreeList = *(size_t*)((char*)this->pValues + handle * this->valueSize);
        return handle;
    }
    if (this->hFreeArray >= this->capacity) {
        size_t newCapacity = this->capacity * _MP_GROWTH_FACTOR_NUMERATOR / _MP_GROWTH_FACTOR_DENOMINATOR;
        if (newCapacity == 0) {
            newCapacity = _MP_DEFAULT_INITIAL_CAPACITY;
        }
        if (_mpResize(this, newCapacity) != 0) {
            return MP_INVALID_HANDLE;
        }
    }
    handle = this->hFreeArray;
    this->hFreeArray += 1;
    return handle;
}

void mpFree(void* pMemPool, size_t handle)
{
    _MpDummy* this = pMemPool;
    if (handle != MP_INVALID_HANDLE) {
        *(size_t*)((char*)this->pValues + handle * this->valueSize) = this->hFreeList;
        this->hFreeList = handle;
    }
}

size_t mpCapacity(void* pMemPool)
{
    _MpDummy* this = pMemPool;
    return this->capacity;
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
