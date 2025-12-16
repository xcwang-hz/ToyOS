#include "types.h"
// #include "Assertions.h"
#include "kmalloc.h"
#include <AK/Types.h>

extern "C" {

// void memcpy(void *dest_ptr, const void *src_ptr, dword n)
// {
//     dword dest = (dword)dest_ptr;
//     dword src = (dword)src_ptr;
//     // FIXME: Support starting at an unaligned address.
//     if (!(dest & 0x3) && !(src & 0x3) && n >= 12) {
//         size_t dwords = n / sizeof(dword);
//         asm volatile(
//             "rep movsl\n"
//             : "=S"(src), "=D"(dest)
//             : "S"(src), "D"(dest), "c"(dwords)
//             : "memory"
//         );
//         n -= dwords * sizeof(dword);
//         if (n == 0)
//             return;
//     }
//     asm volatile(
//         "rep movsb\n"
//         :: "S"(src), "D"(dest), "c"(n)
//         : "memory"
//     );
// }

// void strcpy(char* dest, const char *src)
// {
//     while ((*dest++ = *src++) != '\0');
// }

void* memset(void* dest_ptr, byte c, dword n)
{
#if defined(I386)    
    dword dest = (dword)dest_ptr;
    // FIXME: Support starting at an unaligned address.
    if (!(dest & 0x3) && n >= 12) {
        size_t dwords = n / sizeof(dword);
        dword expanded_c = c;
        expanded_c |= expanded_c << 8;
        expanded_c |= expanded_c << 16;
        asm volatile(
            "rep stosl\n"
            : "=D"(dest)
            : "D"(dest), "c"(dwords), "a"(expanded_c)
            : "memory"
        );
        n -= dwords * sizeof(dword);
        if (n == 0)
            return dest_ptr;
    }
    asm volatile(
        "rep stosb\n"
        : "=D" (dest), "=c" (n)
        : "0" (dest), "1" (n), "a" (c)
        : "memory"
    );
#elif defined(WASM)
    byte* p = (byte*)dest_ptr;
    if (n >= 4) {
        dword expanded_c = (dword)c & 0xFF;
        expanded_c |= expanded_c << 8;
        expanded_c |= expanded_c << 16;
        expanded_c |= expanded_c << 24;

        size_t dwords = n / 4;
        dword* pd = (dword*)p;
        while (dwords--)
            *pd++ = expanded_c;
        
        p = (byte*)pd;
        n %= 4; // Remaining bytes (0-3)
    }

    while (n--)
        *p++ = c;
#else
    #error "Unknown architecture"
#endif            
    return dest_ptr;
}

// char* strrchr(const char* str, int ch)
// {
//     char *last = nullptr;
//     char c;
//     for (; (c = *str); ++str) {
//         if (c == ch)
//             last = const_cast<char*>(str);
//     }
//     return last;
// }

// dword strlen(const char* str)
// {
//     dword len = 0;
//     while (*(str++))
//         ++len;
//     return len;
// }

// int strcmp(const char *s1, const char *s2)
// {
//     for (; *s1 == *s2; ++s1, ++s2) {
//         if (*s1 == 0)
//             return 0;
//     }
//     return *(const byte*)s1 < *(const byte*)s2 ? -1 : 1;
// }

// char* strdup(const char *str)
// {
//     dword len = strlen(str);
//     char *s = (char*)kmalloc(len);
//     memcpy(s, str, len);
//     return s;
// }

// int memcmp(const void* v1, const void* v2, size_t n)
// {
//     auto* s1 = (const byte*)v1;
//     auto* s2 = (const byte*)v2;
//     while (n-- > 0) {
//         if (*s1++ != *s2++)
//             return s1[-1] < s2[-1] ? -1 : 1;
//     }
//     return 0;
// }

// void __cxa_pure_virtual() NORETURN;
// void __cxa_pure_virtual()
// {
//     ASSERT_NOT_REACHED();
// }

}
