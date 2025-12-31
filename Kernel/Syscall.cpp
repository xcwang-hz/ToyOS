// #include "Process.h"
#include "Syscall.h"
// #include "Console.h"
#include "Scheduler.h"
#include "entry.h"
#ifdef I386
#include "i386.h"
extern "C" void syscall_entry(RegisterDump&);
extern "C" void syscall_ISR();
extern volatile RegisterDump* syscallRegDump;

asm(
    ".globl syscall_ISR \n"
    "syscall_ISR:\n"
    "    pusha\n"
    "    pushw %ds\n"
    "    pushw %es\n"
    "    pushw %fs\n"
    "    pushw %gs\n"
    "    pushw %ss\n"
    "    pushw %ss\n"
    "    pushw %ss\n"
    "    pushw %ss\n"
    "    pushw %ss\n"
    "    popw %ds\n"
    "    popw %es\n"
    "    popw %fs\n"
    "    popw %gs\n"
    "    mov %esp, %eax\n"
    "    push %eax\n"
    "    call syscall_entry\n"
    "    add $4, %esp\n"
    "    popw %gs\n"
    "    popw %gs\n"
    "    popw %fs\n"
    "    popw %es\n"
    "    popw %ds\n"
    "    popa\n"
    "    iret\n"
);
#endif

namespace Syscall {
    void initialize()
    {
#ifdef I386        
        register_user_callable_interrupt_handler(0x80, syscall_ISR);
        kprintf("syscall: int 0x80 handler installed\n");
#endif        
    }
#ifdef I386
    static dword handle(RegisterDump& regs, dword function, dword arg1, dword arg2, dword arg3)
#else
    dword handle(dword function, dword arg1, dword arg2, dword arg3)
#endif    
    {
        switch (function) {
        case Syscall::SC_putch:
            terminal1->on_char(((dword)arg1 & 0xff));
            terminal1->paint();
            break;
        case Syscall::SC_read:
            return Keyboard::the().read_char();
        default:
            // kprintf("<%u> int0x80: Unknown function %u requested {%x, %x, %x}\n", current->pid(), function, arg1, arg2, arg3);
            break;            
        }
        return 0;
    }
}

#ifdef I386
void syscall_entry(RegisterDump& regs)
{
    dword function = regs.eax;
    dword arg1 = regs.edx;
    dword arg2 = regs.ecx;
    dword arg3 = regs.ebx;
    regs.eax = Syscall::handle(regs, function, arg1, arg2, arg3);
}
#endif
