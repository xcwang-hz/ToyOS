#include <AK/Types.h>
#include <Kernel/kprintf.h>
#include <Kernel/entry.h>
#include <Kernel/Scheduler.h>
#include <Kernel/Process.h>
#include <arch/wasm/entry.h>

bool initialized = false;
const int SCREEN_WIDTH = 1024;
const int SCREEN_HEIGHT = 768;
uint32_t wasm_framebuffer[SCREEN_WIDTH * SCREEN_HEIGHT];

extern "C" volatile int32_t wasm_pending_key = 0;
extern "C" void wasm_entry(uint32_t ramdisk_addr, uint32_t ramdisk_size) 
{
    if (!initialized) {
        kernel_entry(SCREEN_WIDTH, SCREEN_HEIGHT, (uint32_t)wasm_framebuffer, 
            (uint8_t*)ramdisk_addr);
        js_canvas_init(wasm_framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT);
        initialized = true;
    }

    if (!current)
        return;

    if (current->m_is_first_time) {
        current->m_is_first_time = false;
        if (current->m_entry) {
            current->m_entry();
        }
    }
    Scheduler::timer_tick();    
}

void __assertion_failed(const char* msg, const char* file, unsigned line, const char* func)
{
    kprintf("ASSERTION FAILED: %s\n%s:%u in %s\n", msg, file, line, func);
    __builtin_trap();
}

void check_wasm_key() {
    if (wasm_pending_key != 0) {
        int key = wasm_pending_key;
        wasm_pending_key = 0;
        Keyboard::the().handle_scancode((byte)key);
    }
}
