#pragma once

#include <AK/Types.h>

#define ENUMERATE_SYSCALLS \
    __ENUMERATE_SYSCALL(putch) \
    __ENUMERATE_SYSCALL(read) \
    __ENUMERATE_SYSCALL(get_arguments) \
    __ENUMERATE_SYSCALL(exit) \
    __ENUMERATE_SYSCALL(yield) \

#ifdef WASM
extern "C" volatile int32_t wasm_syscall_params[];
extern "C" dword js_syscall_handle(dword function, dword arg1, dword arg2, dword arg3);
extern "C" dword js_syscall_handle_wait(dword function, dword arg1, dword arg2, dword arg3);
extern "C" dword internal_wasm_handle(dword function, dword arg1, dword arg2, dword arg3);
#endif    

namespace Syscall {

enum Function {
#undef __ENUMERATE_SYSCALL
#define __ENUMERATE_SYSCALL(x) SC_ ##x ,
    ENUMERATE_SYSCALLS
#undef __ENUMERATE_SYSCALL
};

void initialize();

#ifdef WASM
inline dword syscall_handle(dword function, dword arg1, dword arg2, dword arg3)
{
    if ((function==Syscall::SC_yield) || (function==Syscall::SC_read)) {
        wasm_syscall_params[0] = 0;
        dword result = 0;
        while (true) {
            result = js_syscall_handle_wait(function, arg1, arg2, arg3);
            if (wasm_syscall_params[0])
                return wasm_syscall_params[4];
        }
        return result;
    }
    else
        return js_syscall_handle(function, arg1, arg2, arg3);    
}
#endif

inline dword invoke(Function function)
{
    dword result;
#ifdef I386    
    asm volatile("int $0x80":"=a"(result):"a"(function):"memory");
#else
    result = syscall_handle(function, 0, 0, 0);
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
    result = syscall_handle(function, (dword)arg1, 0, 0);
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
    result = syscall_handle(function, (dword)arg1, (dword)arg2, 0);
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
    result = syscall_handle(function, (dword)arg1, (dword)arg2, (dword)arg3);
#endif
    return result;
}
}

#undef __ENUMERATE_SYSCALL
#define __ENUMERATE_SYSCALL(x) using Syscall::SC_ ##x;
    ENUMERATE_SYSCALLS
#undef __ENUMERATE_SYSCALL
#define syscall Syscall::invoke