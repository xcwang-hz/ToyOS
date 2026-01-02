/* linker.ld */
ENTRY(_start)

SECTIONS
{
    /* Load the executable at a fixed address (e.g. 0x08048000) */
    . = 0x08048000;

    .text : ALIGN(4K)
    {
        *(.text)
    }

    .rodata : ALIGN(4K)
    {
        *(.rodata)
    }

    .data : ALIGN(4K)
    {
        *(.data)
    }

    .bss : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }
}