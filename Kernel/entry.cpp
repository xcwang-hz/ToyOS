#include "Size.h"
#include <AK/Types.h>
#include <Terminal.h>
#include "kprintf.h"

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

#ifdef I386
#include "i386.h"
#include "PIC.h"
#include "Keyboard.h"

Keyboard* keyboard;
#else
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
// Static buffer for Wasm
uint32_t wasm_framebuffer[SCREEN_WIDTH * SCREEN_HEIGHT];
extern "C" void canvas_refresh(uint32_t* ptr, int width, int height);
#endif

extern "C" void kernel_entry(uint32_t magic, multiboot_info_t* mbd) {
#ifdef I386
    if (magic != 0x2BADB002)
        return; // Not Multiboot
    if (!(mbd->flags & (1 << 12)))
        return; // Not in graphics mode

    int w = mbd->framebuffer_width;
    int h = mbd->framebuffer_height;        

    cli();
    // RTC::initialize();
    PIC::initialize();
    //gdt_init();
    idt_init();
    keyboard = new Keyboard;
#else
    (void)magic;
    (void)mbd;
    int w = SCREEN_WIDTH;
    int h = SCREEN_HEIGHT;    
#endif        

    Size size(w, h);
#ifdef I386    
    RGBA32* fb_ptr = (RGBA32*)((uint32_t)mbd->framebuffer_addr);
#else
    RGBA32* fb_ptr = (RGBA32*)wasm_framebuffer;
#endif        

    dbgprintf("kernel_entry: fb_ptr = %p\n", fb_ptr);
    Terminal::the().create_window(size, fb_ptr);
    Terminal::the().on_char('T');
    Terminal::the().on_char('o');
    Terminal::the().on_char('y');
    Terminal::the().on_char('O');
    Terminal::the().on_char('S');
    Terminal::the().paint();

#ifdef I386
    sti();
    for (;;)
        asm("hlt");
#else
    canvas_refresh(wasm_framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT);
#endif    
}