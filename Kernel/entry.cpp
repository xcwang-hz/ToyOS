#include <AK/Types.h>

/* * Multiboot Info Structure (Partial definition)
 * We need the framebuffer fields which start at offset 88 (0x58).
 */
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

extern "C" void kernel_entry(uint32_t magic, multiboot_info_t* mbd) {
    if (magic != 0x2BADB002)
        return; // Not Multiboot

    if (!(mbd->flags & (1 << 12)))
        return; // Not in graphics mode

    uint32_t address = (uint32_t)mbd->framebuffer_addr;
}