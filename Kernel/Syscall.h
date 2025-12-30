#pragma once

#include <AK/Types.h>
#include "Keyboard.h"
#include "Terminal.h"
#include "entry.h"

namespace Syscall {
    enum Function {
        SC_putch,
        SC_read
    };

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
    // asm volatile("int $0x80":"=a"(result):"a"(function),"d"((dword)arg1):"memory");
    
    if (function == Function::SC_putch) {
        terminal1->on_char(((dword)arg1 & 0xff));
        terminal1->paint();
    }
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
    // asm volatile("int $0x80":"=a"(result):"a"(function),"d"((dword)arg1),"c"((dword)arg2),"b"((dword)arg3):"memory");
    if (function == Function::SC_read) {
        result = Keyboard::the().read_char();
    }

    return result;
}
}

#define syscall Syscall::invoke