#pragma once

#include <AK/Types.h>

#define ENUMERATE_SYSCALLS \
    __ENUMERATE_SYSCALL(putch) \
    __ENUMERATE_SYSCALL(read) \
    __ENUMERATE_SYSCALL(get_arguments) \
    __ENUMERATE_SYSCALL(exit) \

#ifdef WASM
extern "C" dword wasm_handle(dword function, dword arg1, dword arg2, dword arg3);
#endif    

namespace Syscall {

enum Function {
#undef __ENUMERATE_SYSCALL
#define __ENUMERATE_SYSCALL(x) SC_ ##x ,
    ENUMERATE_SYSCALLS
#undef __ENUMERATE_SYSCALL
};

void initialize();

inline dword invoke(Function function)
{
    dword result;
#ifdef I386    
    asm volatile("int $0x80":"=a"(result):"a"(function):"memory");
#else 
    result = wasm_handle(function, 0, 0, 0);
#endif
    return result;
}

template<typename T1>
inline dword invoke(Function function, T1 arg1)
{
    dword result;
#ifdef I386    
    asm volatile("int $0x80":"=a"(result):"a"(function),"d"((dword)arg1):"memory");
#else    
    result = wasm_handle(function, (dword)arg1, 0, 0);
#endif
    return result;
}

template<typename T1, typename T2>
inline dword invoke(Function function, T1 arg1, T2 arg2)
{
    dword result;
#ifdef I386    
    asm volatile("int $0x80":"=a"(result):"a"(function),"d"((dword)arg1),"c"((dword)arg2):"memory");
#else 
    result = wasm_handle(function, (dword)arg1, (dword)arg2, 0);
#endif
    return result;
}

template<typename T1, typename T2, typename T3>
inline dword invoke(Function function, T1 arg1, T2 arg2, T3 arg3)
{
    dword result;
#ifdef I386    
    asm volatile("int $0x80":"=a"(result):"a"(function),"d"((dword)arg1),"c"((dword)arg2),"b"((dword)arg3):"memory");
#else
    result = wasm_handle(function, (dword)arg1, (dword)arg2, (dword)arg3);
#endif
    return result;
}
}

#undef __ENUMERATE_SYSCALL
#define __ENUMERATE_SYSCALL(x) using Syscall::SC_ ##x;
    ENUMERATE_SYSCALLS
#undef __ENUMERATE_SYSCALL
#define syscall Syscall::invoke