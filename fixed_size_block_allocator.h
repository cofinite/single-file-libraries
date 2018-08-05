/*
fixed_size_block_allocator.h - public domain - github.com/cofinite

In exactly one source file, put:
    #define FSBA_IMPLEMENTATION
    #include "fixed_size_block_allocator.h"
    
Other source or header files should have just:
    #include "fixed_size_block_allocator.h"


The purpose of this library is to provide dependency-free fixed-size block
allocation for C89, in a single file. This library does not depend on `malloc`,
but instead takes memory from the user.

To create a fixed-size block allocator, call `fsbaEmplaceAllocator` and pass to
it some amount of memory:

    MyObjectType mem[100];
    FsbaAllocator* allocator = fsbaEmplaceAllocator(
        mem,
        sizeof mem,
        sizeof *mem,
        alignof(MyObjectType),
        NULL);

You can then call `fsbaAllocate` and `fsbaFree` to allocate and free blocks of
memory:

    MyObjectType* obj = fsbaAllocate(allocator);
    fsbaFree(allocator, obj);

Once you are done using the allocator, simply take care of the memory that was
passed to `fsbaEmplaceAllocator`. If this memory has static or automatic
storage duration, nothing needs to be done.

More detailed documentation follows.

LICENSE

See end of file for license information.

*/

#ifndef FSBA_INCLUDE_FIXED_SIZE_BLOCK_ALLOCATOR_H
#define FSBA_INCLUDE_FIXED_SIZE_BLOCK_ALLOCATOR_H

#include <stddef.h>

/*! @brief Opaque allocator object.
 *  
 *  Opaque allocator object.
 */
typedef struct FsbaAllocator FsbaAllocator;

/*! @brief Emplaces an allocator in the given memory.
 *  
 *  This function constructs an allocator in-place within the memory passed to
 *  it. The rest of the memory is used by the allocator to allocate fixed-size
 *  blocks of memory.
 *  
 *  The caller is responsible for the memory passed to this function. This
 *  interface provides no destructor that would need to be called when you are
 *  finished with the allocator.
 *  
 *  @param[in] pMem Pointer to the memory to be used by the allocator.
 *  
 *  @param[in] memSize The size of the memory pointed to by `pMem`.
 *  
 *  @param[in] blockSize The fixed size of the memory blocks to be allocated.
 *  
 *  @param[in] blockAlign The alignment requirement of the memory blocks.
 *  
 *  @param[out] pBlockCount Where to store the maximum number of blocks that
 *  can be allocated at once, or `NULL`.
 *  
 *  @return A handle to the allocator, or `NULL` if not given enough memory.
 *  
 *  @remarks The allocator has internal requirements for `blockSize` and
 *  `blockAlign`. Certain input values may result in increased memory usage due
 *  to padding.
 */
FsbaAllocator* fsbaEmplaceAllocator(
    void* pMem,
    size_t memSize,
    size_t blockSize,
    size_t blockAlign,
    size_t* pBlockCount);

/*! @brief Allocates a memory block.
 *  
 *  This function allocates a memory block of the size specified during the
 *  creation of the allocator.
 *  
 *  @param[in] pAllocator Handle to the allocator from which to request the
 *  memory block.
 *  
 *  @return A pointer to the memory block, or `NULL` if the allocator is out of
 *  memory.
 */
void* fsbaAllocate(FsbaAllocator* pAllocator);

/*! @brief Frees a memory block.
 *  
 *  This function frees a memory block that has previously been returned by a
 *  call to `fsbaAllocate`.
 *  
 *  @param[in] pAllocator Handle to the allocator from which the memory block
 *  was previously requested.
 *  
 *  @param[in] pBlock Pointer to the memory block to be freed.
 *  This must have been previously returned by a call to `fsbaAllocate`, using
 *  the same allocator.
 */
void fsbaFree(FsbaAllocator* pAllocator, void* pBlock);

/*! @brief Returns the size of an allocator.
 *  
 *  This function returns the size of an allocator object. Can be good to know
 *  when optimizing the memory given to an allocator.
 *  
 *  @return sizeof(FsbaAllocator)
 */
size_t fsbaAllocatorSize(void);

/*! @brief Returns the alignment requirement of an allocator.
 *  
 *  This function returns the alignment requirement of an allocator object. Can
 *  be good to know when optimizing the memory given to an allocator.
 *  
 *  @return alignof(FsbaAllocator)
 */
size_t fsbaAllocatorAlignment(void);

#endif /* FSBA_INCLUDE_FIXED_SIZE_BLOCK_ALLOCATOR_H */



#ifdef FSBA_IMPLEMENTATION

struct FsbaAllocator {
    char* pFreeMemBegin;
    char* pFreeMemEnd;
    size_t blockSize;
    void** pFreeBlock;
};

#define fsba_alignof(type) offsetof(struct {char x; type y;}, y)

static void* fsba_alignUp(void* ptr, size_t align)
{
    return (char*)ptr + (align - ((((size_t)ptr - 1) % align) + 1));
}

static size_t fsba_roundUp(size_t num, size_t multiple)
{
    return num + (multiple - (((num - 1) % multiple) + 1));
}

static size_t fsba_roundDown(size_t num, size_t multiple)
{
    return num - (num % multiple);
}

static size_t fsba_GCD(size_t a, size_t b)
{
    size_t temp;
    while (b != 0) {
        temp = a % b;
        a = b;
        b = temp;
    }
    return a;
}

static size_t fsba_LCM(size_t a, size_t b)
{
    return (a * b) / fsba_GCD(a, b);
}

FsbaAllocator* fsbaEmplaceAllocator(
    void* pMem,
    size_t memSize,
    size_t blockSize,
    size_t blockAlign,
    size_t* pBlockCount)
{
    FsbaAllocator* pAllocator;
    char* pBlockMemBegin;
    size_t memUsed;
    
    if (pMem == NULL) goto out_of_memory;
    
    /* place the allocator in the first address aligned to hold it */
    pAllocator = fsba_alignUp(pMem, fsba_alignof(FsbaAllocator));
    
    /* blocks must be aligned at least as strictly as pointers */
    blockAlign = fsba_LCM(blockAlign, fsba_alignof(void*));
    
    /*  It would have been possible to allow blocks to be aligned less strictly
     *  than pointers, while still being able to store pointers. This was not
     *  implemented because it would have resulted in a performance penalty for
     *  allocation and deallocation.
     */
    
    /* blocks must be large enough to hold pointers */
    if (blockSize < sizeof(void*)) blockSize = sizeof(void*);
    blockSize = fsba_roundUp(blockSize, blockAlign);
    
    /* block memory begins at first aligned address after the allocator */
    pBlockMemBegin = fsba_alignUp(pAllocator + 1, blockAlign);
    
    memUsed = (size_t)(pBlockMemBegin - (char*)pMem);
    if (memUsed > memSize) goto out_of_memory;
    
    /* correct the size of the memory down to its effective size */
    memSize = fsba_roundDown(memSize - memUsed, blockSize);
    
    /* if the total number of allocatable blocks was requested, give it */
    if (pBlockCount != NULL) *pBlockCount = memSize / blockSize;
    
    pAllocator->pFreeMemBegin = pBlockMemBegin;
    pAllocator->pFreeMemEnd = pBlockMemBegin + memSize;
    pAllocator->blockSize = blockSize;
    pAllocator->pFreeBlock = NULL;
    
    return pAllocator;
    
out_of_memory:
    
    if (pBlockCount != NULL) *pBlockCount = 0;
    return NULL;
}

void* fsbaAllocate(FsbaAllocator* pAllocator)
{
    void* out = pAllocator->pFreeBlock;
    if (out != NULL) {
        pAllocator->pFreeBlock = *pAllocator->pFreeBlock;
        return out;
    }
    if (pAllocator->pFreeMemBegin >= pAllocator->pFreeMemEnd) {
        return NULL;
    }
    out = pAllocator->pFreeMemBegin;
    pAllocator->pFreeMemBegin += pAllocator->blockSize;
    return out;
}

void fsbaFree(FsbaAllocator* pAllocator, void* pBlock)
{
    if (pBlock == NULL) return;
    *(void**)pBlock = pAllocator->pFreeBlock;
    pAllocator->pFreeBlock = pBlock;
}

size_t fsbaAllocatorSize(void)
{
    return sizeof(FsbaAllocator);
}

size_t fsbaAllocatorAlignment(void)
{
    return fsba_alignof(FsbaAllocator);
}

#undef fsba_alignof

#endif /* FSBA_IMPLEMENTATION */

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