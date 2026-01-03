#include <AK/Types.h>
#include <Kernel/kprintf.h>
#include <Kernel/StdLib.h>
#include <Kernel/entry.h>
#include <arch/i386/i386.h>
#include "i8253.h"
#include "PIC.h"

struct multiboot_info_t {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
    
    /* Framebuffer info (Added in Multiboot 1) */
    uint64_t framebuffer_addr;  // Physical address of the screen
    uint32_t framebuffer_pitch; // Bytes per line
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t  framebuffer_bpp;   // Bits per pixel (e.g., 32)
    uint8_t  framebuffer_type;
};

struct multiboot_module_t {
    uint32_t mod_start; // Physical address where module starts
    uint32_t mod_end;   // Physical address where module ends
    uint32_t string;    // Pointer to command line string (e.g., "clear")
    uint32_t reserved;
};

int find_initramfs_from_multiboot(multiboot_info_t* mbd) {
    // Check flag bit 3 (mods_* fields are valid)
    if (!(mbd->flags & (1 << 3))) {
        kprintf("Multiboot: No modules found.\n");
        return -1;
    }

    multiboot_module_t* mods = (multiboot_module_t*)mbd->mods_addr;
    for (int i = 0; i < (int)mbd->mods_count; i++) {
        uint32_t mod_start = mods[i].mod_start;
        uint32_t mod_end = mods[i].mod_end;
        uint32_t size = mod_end - mod_start;
        char* cmdline = (char*)mods[i].string;
        if ((strcmp(cmdline, "initramfs")!=0) || (size==0))
            continue;

        kprintf("CPIO module found: %s @ 0x%x (Size: %d)\n", cmdline, mod_start, size);
        return i;
    }

    return -1;
}

extern "C" void i386_entry(uint32_t magic, multiboot_info_t* mbd) {
    if (magic != 0x2BADB002)
        return; // Not Multiboot
    if (!(mbd->flags & (1 << 12)))
        return; // Not in graphics mode

    multiboot_module_t* mods = (multiboot_module_t*)mbd->mods_addr;
    int index = find_initramfs_from_multiboot(mbd);
    if (index < 0)
        return;

    cli();
    // RTC::initialize();
    PIC::initialize();
    gdt_init();
    idt_init();
    PIT::initialize();
    kernel_entry(mbd->framebuffer_width, mbd->framebuffer_height, (uint32_t)mbd->framebuffer_addr, 
    (uint8_t*)mods[index].mod_start);
    sti();
    for (;;)
        asm("hlt");
}