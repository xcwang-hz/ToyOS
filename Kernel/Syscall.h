#pragma once

#include <AK/Types.h>

#define ENUMERATE_SYSCALLS \
    __ENUMERATE_SYSCALL(putch) \
    __ENUMERATE_SYSCALL(read) \

namespace Syscall {

enum Function {
#undef __ENUMERATE_SYSCALL
#define __ENUMERATE_SYSCALL(x) SC_ ##x ,
    ENUMERATE_SYSCALLS
#undef __ENUMERATE_SYSCALL
};

void initialize();

// inline dword invoke(Function function)
// {
//     dword result;
//     asm volatile("int $0x80":"=a"(result):"a"(function):"memory");
//     return result;
// }

template<typename T1>
inline dword invoke(Function function, T1 arg1)
{
    dword result;
#ifdef I386    
    asm volatile("int $0x80":"=a"(result):"a"(function),"d"((dword)arg1):"memory");
#else    
    // if (function == Function::SC_putch) {
    //     terminal1->on_char(((dword)arg1 & 0xff));
    //     terminal1->paint();
    // }
#endif
    return result;
}

template<typename T1, typename T2>
inline dword invoke(Function function, T1 arg1, T2 arg2)
{
    dword result;
    // asm volatile("int $0x80":"=a"(result):"a"(function),"d"((dword)arg1),"c"((dword)arg2):"memory");
    return result;
}

template<typename T1, typename T2, typename T3>
inline dword invoke(Function function, T1 arg1, T2 arg2, T3 arg3)
{
    dword result;
#ifdef I386    
    asm volatile("int $0x80":"=a"(result):"a"(function),"d"((dword)arg1),"c"((dword)arg2),"b"((dword)arg3):"memory");
#else
    // if (function == Function::SC_read) {
    //     result = Keyboard::the().read_char();
    // }
#endif
    return result;
}
}

#undef __ENUMERATE_SYSCALL
#define __ENUMERATE_SYSCALL(x) using Syscall::SC_ ##x;
    ENUMERATE_SYSCALLS
#undef __ENUMERATE_SYSCALL
#define syscall Syscall::invoke