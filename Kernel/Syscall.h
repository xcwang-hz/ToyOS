#pragma once

#include <AK/Types.h>
#include "Terminal.h"
#include "entry.h"

namespace Syscall {
    enum Function {
        SC_putch
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
}

#define syscall Syscall::invoke