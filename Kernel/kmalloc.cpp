#include <Kernel/types.h>
#include <Kernel/kprintf.h>
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

alignas(16) static byte s_heap[5* 1024 * 1024]; // 5MB
static size_t s_heap_ptr = 0;

static inline size_t align_up(size_t val, size_t alignment) {
    return (val + alignment - 1) & ~(alignment - 1);
}

void* kmalloc_impl(size_t size)
{
    if (size == 0) 
        return nullptr;

    static bool s_printed = false;
    if (!s_printed) {
        kprintf("[Debug] s_heap base address: %p\n", s_heap);
        kprintf("[Debug] s_heap end address:  %p\n", s_heap + sizeof(s_heap));
        s_printed = true;
    }

    size_t current_addr = (size_t)&s_heap[s_heap_ptr];
    size_t aligned_addr = align_up(current_addr, 16);
    size_t padding = aligned_addr - current_addr;
    if (s_heap_ptr + padding + size > sizeof(s_heap)) {
        // kprintf("KERNEL OOM! Request: %zu, Heap ptr: %zu\n", size, s_heap_ptr);
        return nullptr;
    }        

    s_heap_ptr += padding + size;
    void* ptr = (void*)aligned_addr;
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
    return kmalloc_impl(size);
}