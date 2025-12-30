#include "Size.h"
#include <AK/Types.h>
#include <Terminal.h>
#include <Scheduler.h>
#include <Process.h>
#include "kprintf.h"
#include "system.h"
#ifdef I386
#include "i386.h"
#include "i8253.h"
#include "PIC.h"
#include "Keyboard.h"
#else
#include "entry.h"
uint32_t wasm_framebuffer[SCREEN_WIDTH * SCREEN_HEIGHT];
#endif

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

extern "C" volatile int32_t js_pending_key = 0;
extern void shell_main();
Terminal* terminal1 = nullptr;
Terminal* terminal2 = nullptr;
system_t system;
bool initialized = false;
void check_js_key() {
    if (js_pending_key != 0) {
        int key = js_pending_key;
        js_pending_key = 0;
        Keyboard::the().handle_scancode((byte)key);
    }
}

void task1_entry() {
    while (true) {
        terminal1->on_char('A');
        terminal1->paint(); 
        Scheduler::yield(); 
        
        for (int i = 0; i < 1000000; i++); 
    }
}

void task2_entry() {
    while (true) {
        terminal2->on_char('B');
        terminal2->paint();
        Scheduler::yield();
        for (int i = 0; i < 2000000; i++);
    }
}

Keyboard* keyboard;
extern "C" void kernel_entry(uint32_t magic, multiboot_info_t* mbd) {
    if (!initialized) {
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
        PIT::initialize();
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

        Process::initialize();

        terminal2 = new Terminal({w/2,h/2});
        terminal1 = new Terminal({0,0});
        terminal1->create_window(size, fb_ptr);
        terminal2->create_window(size, fb_ptr);

        Process::create_kernel_process("Shell", shell_main);
        Process::create_kernel_process("Background", task2_entry);

        terminal1->paint();
        // terminal2->paint();
        initialized = true;

#ifdef WASM
        canvas_init(wasm_framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT);
#endif        
    }

#ifdef I386
    sti();
    for (;;)
        asm("hlt");
#else
    if (!current)
        return;

    check_js_key();    
    if (current->m_is_first_time) {
        current->m_is_first_time = false;
        if (current->m_entry) {
            current->m_entry();
        }
    }
    Scheduler::timer_tick();    
#endif
}