#include "types.h"
// #include "kmalloc.h"
// #include "StdLib.h"
// #include "i386.h"
// #include "system.h"
// #include "Process.h"
// #include "Scheduler.h"
// #include <AK/Assertions.h>

namespace std {
    enum class align_val_t : size_t {};
}

static byte s_heap[4* 1024 * 1024]; // 4MB
static size_t s_heap_ptr = 0;

void* kmalloc_impl(size_t size)
{
    void* ptr = &s_heap[s_heap_ptr];
    s_heap_ptr += size;
    return ptr;
}

void kfree(void* ptr)
{
    // free(ptr);
}

void* operator new(size_t size, std::align_val_t al)
{
    (void)al; 
    return kmalloc_impl(size);
}

void operator delete(void* ptr, std::align_val_t al)
{
    (void)al;
    //kfree(ptr);
}

void* operator new[](size_t size, std::align_val_t al)
{
    (void)al;
    return kmalloc_impl(size);
}

void operator delete[](void* ptr, std::align_val_t al)
{
    (void)al;
    //kfree(ptr);
}

void* operator new(size_t size)
{
    return kmalloc_impl(size);
}

void* operator new[](size_t size)
{
    return kmalloc_impl(size);
}

void operator delete(void* ptr)
{
    // return kfree(ptr);
}

void operator delete[](void* ptr)
{
    // return kfree(ptr);
}

void operator delete(void* ptr, unsigned int)
{
    // return kfree(ptr);
}

void operator delete[](void* ptr, unsigned int)
{
    // return kfree(ptr);
}

void* kmalloc_eternal(size_t size)
{
    void* ptr = &s_heap[s_heap_ptr];
    s_heap_ptr += size;
    return ptr;
}