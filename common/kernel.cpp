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

#ifdef CONSOLE
extern "C" void console_log(const char* ptr);
#endif // End CONSOLE

#ifdef GRAPHICS


#ifdef WASM
    // A=FF, B=00, G=00, R=FF -> In memory: FF 00 00 FF
    uint32_t COLOR_RED   = 0xFF0000FF; 
    // A=FF, B=FF, G=FF, R=FF -> In memory: FF FF FF FF
    uint32_t COLOR_WHITE = 0xFFFFFFFF;
#else
    uint32_t COLOR_RED   = 0x00FF0000;
    uint32_t COLOR_WHITE = 0x00FFFFFF;
#endif

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

#ifdef WASM
    // Static buffer for Wasm
    uint32_t wasm_framebuffer[SCREEN_WIDTH * SCREEN_HEIGHT];
    extern "C" void canvas_refresh(uint32_t* ptr, int width, int height);
#endif

void put_pixel(multiboot_info_t* mbd, uint32_t x, uint32_t y, uint32_t color) {
    #ifdef I386
        if (mbd && (mbd->flags & (1 << 12))) {
            uint32_t bpp = mbd->framebuffer_bpp / 8;
            uintptr_t addr = (uintptr_t)mbd->framebuffer_addr + (y * mbd->framebuffer_pitch) + (x * bpp);
            *(volatile uint32_t*)addr = color;
        }
    #elif defined(WASM)
        (void)mbd; // Silence warning
        if (x < SCREEN_WIDTH && y < SCREEN_HEIGHT) {
            wasm_framebuffer[y * SCREEN_WIDTH + x] = color;
        }
    #endif
}

void graphics_draw_rect(multiboot_info_t* mbd, int x, int y, int w, int h, uint32_t color) {
    for (int row = 0; row < h; row++) {
        for (int col = 0; col < w; col++) {
            put_pixel(mbd, x + col, y + row, color);
        }
    }
}

#endif // End GRAPHICS

extern "C" void kernel_entry(uint32_t magic, multiboot_info_t* mbd) {
    (void)magic; // Silence warning
 
    #ifdef CONSOLE
        (void)mbd; // Silence warning
        console_log("ToyOS: ");
    #endif

    #ifdef GRAPHICS
        // Draw White Background
        #ifdef I386
            uint32_t w = mbd->framebuffer_width;
            uint32_t h = mbd->framebuffer_height;
        #else
            (void)mbd; // Silence warning
            uint32_t w = SCREEN_WIDTH;
            uint32_t h = SCREEN_HEIGHT;
        #endif

        // Clear Screen White
        graphics_draw_rect(mbd, 0, 0, w, h, COLOR_WHITE);
        
        // Draw Red Box
        // Note: I386 is usually ARGB/BGRA, Wasm is ABGR (Little Endian RGBA)
        // You might need a color conversion macro later.
        graphics_draw_rect(mbd, 350, 250, 100, 100, COLOR_RED);

        #ifdef WASM
            canvas_refresh(wasm_framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT);
        #endif
    #endif    
}