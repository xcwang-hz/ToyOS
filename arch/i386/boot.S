/* Multiboot Header Magic Values */
.set ALIGN,    1<<0             /* align loaded modules on page boundaries */
.set MEMINFO,  1<<1             /* provide memory map via register EBX */
.set FLAGS,    ALIGN | MEMINFO  /* this is the Multiboot 'flag' field */
.set MAGIC,    0x1BADB002       /* 'magic number' lets bootloader find the header */
.set CHECKSUM, -(MAGIC + FLAGS) /* checksum of above, to prove we are multiboot */

/* Declare constants for the multiboot header */
.section .text.multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

/* Reserve a stack for the kernel */
.section .bss
.align 16
stack_bottom:
.skip 16384 /* 16 KiB */
stack_top:

/* The kernel entry point */
.section .text
.global _start
.type _start, @function
_start:
    mov $stack_top, %esp
    xor %ebp, %ebp
    call kernel_entry
    cli
hlt_loop:
    hlt
    jmp hlt_loop

.size _start, . - _start