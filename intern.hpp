/*
intern.hpp - public domain - github.com/cofinite

The purpose of this library is to provide interning for an arbitrary type `T`.
An `interned<T>` is almost like a `T`, with a few notable pros and cons:

+ `interned<T>` wraps a pointer: the size is that of a pointer
+ all `interned<T>` whose values compare equal will refer to the same memory
+ equivalence between `interned<T>` is reduced to pointer equivalence
+ assignment between `interned<T>` is reduced to pointer assignment and 
  reference count fiddling

? `interned<T>` converts to a `const T&`: can be assigned a new value, but held 
  values are immutable

- `interned<T>` require an O(1) lookup when constructed from or assigned a `T`, 
  when destroyed, and potentially when assigned to

In practice, this is useful when `T` is larger than a pointer and many 
semantically equal `T` are expected to exist in memory at once.

Note that the equivalence function for `T` should not throw.


LICENSE

See end of file for license information.

*/

#ifndef INTERN_HPP_INCLUDED
#define INTERN_HPP_INCLUDED

#include <cstddef>
#include <functional>
#include <unordered_map>

template <
    class T,
    class Size  = std::size_t,
    class Hash  = std::hash<T>,
    class Equal = std::equal_to<T>
> class interned {
public:
    typedef T       key_type;
    typedef Size    size_type;
    typedef Hash    hasher;
    typedef Equal   key_equal;
    
    operator const T&()   const { return  ptr->first; }
    const T& operator*()  const { return  ptr->first; }
    const T* operator->() const { return &ptr->first; }
    
    interned(const T& value)            { acquire(&*map.insert({value, 0}).first); }
    interned(const interned& other)     { acquire(other.ptr); }
    interned(interned&& other) noexcept { acquire(other.ptr); }
    
    const interned& operator=(const T& value)            { acquire(&*map.insert({value, 0}).first); return *this; }
    const interned& operator=(const interned& other)     { acquire(other.ptr); return *this; }
    const interned& operator=(interned&& other) noexcept { acquire(other.ptr); return *this; }
    
    ~interned() noexcept { release(); }
    
    bool operator==(const interned& other) const { return ptr == other.ptr; }
    bool operator!=(const interned& other) const { return ptr != other.ptr; }
    
    static Size size() { return map.size(); }
    
private:
    typedef typename std::unordered_map<T, Size, Hash, Equal>::value_type pair_type;
    
    static std::unordered_map<T, Size, Hash, Equal> map;
    pair_type* ptr = nullptr;
    
    void release() {
        if (ptr != nullptr) {
            if (ptr->second <= 1) {
                map.erase(ptr->first);
            }
            else {
                ptr->second -= 1;
            }
        }
    }
    
    void acquire(pair_type* pPair) {
        pPair->second += 1;
        release();
        ptr = pPair;
    }
};

template <class T, class Size, class Hash, class Equal>
std::unordered_map<T, Size, Hash, Equal> interned<T, Size, Hash, Equal>::map;

#endif /* INTERN_HPP_INCLUDED */

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
