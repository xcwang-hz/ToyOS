/* Multiboot Header Magic Values */
.set ALIGN,    1<<0                         /* align loaded modules on page boundaries */
.set MEMINFO,  1<<1                         /* provide memory map via register EBX */
.set VIDEO, 1<<2
.set FLAGS, ALIGN | MEMINFO | VIDEO
.set MAGIC,    0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

/* Declare constants for the multiboot header */
.section .multiboot, "a", @progbits
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

/* * Address fields (unused because we don't set bit 16),
* but we MUST pad them with 0 to reach the video fields at offset 32.
* There are 5 uint32_t fields to skip: 
* header_addr, load_addr, load_end_addr, bss_end_addr, entry_addr
*/
.long 0, 0, 0, 0, 0

/* Video Mode Preferences (Offset 32) */
.long 0    /* Mode Type: 0 = Linear Graphics, 1 = Text */
.long 1024  /* Width */
.long 768  /* Height */
.long 32   /* Depth (Bits Per Pixel) */

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

    /* Push Multiboot Info Pointer (EBX) and Magic (EAX) */
    push %ebx
    push %eax    
    call i386_entry
    cli
hlt_loop:
    hlt
    jmp hlt_loop

.size _start, . - _start