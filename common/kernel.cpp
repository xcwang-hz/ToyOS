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

/* Define color constants (Assuming 32-bit ARGB or BGRA) */
// Format usually: 0x00RRGGBB
const uint32_t COLOR_WHITE = 0x00FFFFFF;
const uint32_t COLOR_RED   = 0x00FF0000; 

void put_pixel(multiboot_info_t* mbd, uint32_t x, uint32_t y, uint32_t color) {
    // Address = Base + (y * pitch) + (x * bytes_per_pixel)
    uint32_t bpp = mbd->framebuffer_bpp / 8; // e.g., 32 bits -> 4 bytes
    uintptr_t address = (uintptr_t)mbd->framebuffer_addr + 
                        (y * mbd->framebuffer_pitch) + 
                        (x * bpp);

    // volatile is crucial to prevent compiler from optimizing out memory writes
    volatile uint32_t* pixel = (volatile uint32_t*)address;
    *pixel = color;
}

void draw_rect(multiboot_info_t* mbd, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    for (uint32_t row = 0; row < h; ++row) {
        for (uint32_t col = 0; col < w; ++col) {
            put_pixel(mbd, x + col, y + row, color);
        }
    }
}

extern "C" {
    void host_console_log(const char* ptr);
}

extern "C" void kernel_entry(uint32_t magic, multiboot_info_t* mbd) {
    //host_console_log("ToyOS: ");

    if (magic != 0x2BADB002)
        return;

    for (uint32_t y = 0; y < mbd->framebuffer_height; ++y) {
        for (uint32_t x = 0; x < mbd->framebuffer_width; ++x) {
            put_pixel(mbd, x, y, COLOR_WHITE);
        }
    }

    /* 4. Draw a RED Square in the Center */
    uint32_t rect_size = 100;
    uint32_t center_x = (mbd->framebuffer_width - rect_size) / 2;
    uint32_t center_y = (mbd->framebuffer_height - rect_size) / 2;

    draw_rect(mbd, center_x, center_y, rect_size, rect_size, COLOR_RED);    
}