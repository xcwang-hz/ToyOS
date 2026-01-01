#include "kprintf.h"

void __assertion_failed(const char* msg, const char* file, unsigned line, const char* func)
{
    kprintf("ASSERTION FAILED: %s\n%s:%u in %s\n", msg, file, line, func);
    __builtin_trap();
}
