#include "kprintf.h"
// #include "Console.h"
#ifdef I386
#include "IO.h"
#else
#include <arch/wasm/entry.h>
#endif
#include <LibC/stdarg.h>
// #include "Process.h"
#include <AK/Types.h>
#include <AK/printf.cpp>

// static void console_putch(char*&, char ch)
// {
//     Console::the().write(*current, (byte*)&ch, 1);
// }

static void debugger_putch(char*&, char ch)
{
#ifdef I386
    IO::out8(0xe9, ch);
#else
    js_debug_char(ch);
#endif    
}

int kprintf(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int ret = printfInternal(debugger_putch, nullptr, fmt, ap);
    va_end(ap);
    return ret;
}

static void buffer_putch(char*& bufptr, char ch)
{
    *bufptr++ = ch;
}

int ksprintf(char* buffer, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int ret = printfInternal(buffer_putch, buffer, fmt, ap);
    buffer[ret] = '\0';
    va_end(ap);
    return ret;
}

extern "C" int dbgprintf(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int ret = printfInternal(debugger_putch, nullptr, fmt, ap);
    va_end(ap);
    return ret;
}
