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

int     mpCreatePool    (void* pMemPool, size_t capacity);
void    mpDestroyPool   (void* pMemPool);
size_t  mpAlloc         (void* pMemPool);
void    mpFree          (void* pMemPool, size_t handle);
size_t  mpCapacity      (void* pMemPool);

#define MP_INVALID_HANDLE ((size_t)-1)

#endif /* MEMORY_POOL_H_INCLUDED */



#ifdef MEMORY_POOL_IMPLEMENTATION

#include <stdlib.h>

#define _MP_GROWTH_FACTOR_NUMERATOR      3
#define _MP_GROWTH_FACTOR_DENOMINATOR    2

typedef MemPool(size_t) _MpDummy;

int _mpResize(_MpDummy* this, size_t capacity)
{
    void* temp = realloc(this->pValues, capacity * this->valueSize);
    if (temp == NULL) {
        return -1;
    }
    this->pValues = temp;
    this->capacity = capacity;
    return 0;
}

int mpCreatePool(void* pMemPool, size_t capacity)
{
    _MpDummy* this = pMemPool;
    return _mpResize(this, capacity);
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
