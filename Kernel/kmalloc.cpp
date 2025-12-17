#include "types.h"
// #include "kmalloc.h"
// #include "StdLib.h"
// #include "i386.h"
// #include "system.h"
// #include "Process.h"
// #include "Scheduler.h"
// #include <AK/Assertions.h>

static byte s_heap[4* 1024 * 1024]; // 4MB
static size_t s_heap_ptr = 0;
void* operator new(size_t size)
{
    void* ptr = &s_heap[s_heap_ptr];
    s_heap_ptr += size;
    return ptr;
}

void* operator new[](size_t size)
{
    void* ptr = &s_heap[s_heap_ptr];
    s_heap_ptr += size;
    return ptr;
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