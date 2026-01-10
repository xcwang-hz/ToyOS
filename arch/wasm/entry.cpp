#include <AK/Types.h>
#include <Kernel/kprintf.h>
#include <Kernel/entry.h>
#include <Kernel/Scheduler.h>
#include <Kernel/Process.h>
#include <arch/wasm/entry.h>
#include <Kernel/Syscall.h>

const int SCREEN_WIDTH = 1024;
const int SCREEN_HEIGHT = 768;
uint32_t wasm_framebuffer[SCREEN_WIDTH * SCREEN_HEIGHT];

extern "C" volatile int32_t wasm_pending_key = 0;
extern "C" volatile int32_t wasm_syscall_params[5] = {0, 0, 0, 0, 0}; // ( func, arg1, arg2, arg3, retval）
extern "C" void wasm_entry(uint32_t ramdisk_addr, uint32_t ramdisk_size) 
{
    volatile int stack_probe = 0;
    kernel_entry(SCREEN_WIDTH, SCREEN_HEIGHT, (uint32_t)wasm_framebuffer, 
        (uint8_t*)ramdisk_addr);
    kprintf("[Debug] Kernel Stack Address: %p\n", &stack_probe);
    js_canvas_init(wasm_framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT);
}

extern "C" void wasm_loop()
{
    while (true) 
    {
        if (!current)
        {
            Scheduler::pick_next();
            
            // If still no process (e.g., system just started or all blocked), 
            // we should ideally idle. For now, we continue to retry.
            if (!current) {
                // Optional: You might want to yield to JS here to prevent freezing the browser
                // if there is absolutely nothing to run (Idle state).
                // js_context_switch(0, 0); 
                continue; 
            }
        }

        if (current->m_entry)
            current->m_entry();
        else if (current->m_user_proc_id > 0)
            js_start_user_process(current->m_user_proc_id);
        Scheduler::timer_tick();
    }
}

extern "C" dword wasm_syscall_handle()
{
    dword result = internal_wasm_handle(wasm_syscall_params[0], wasm_syscall_params[1], wasm_syscall_params[2], wasm_syscall_params[3]);
    wasm_syscall_params[4] = result;
    return result;
}

void __assertion_failed(const char* msg, const char* file, unsigned line, const char* func)
{
    js_console_error("!!! ASSERTION FAILED !!!");
    js_console_error(msg);
    js_console_error(file);
    js_console_error(func);
    //kprintf("ASSERTION FAILED: %s\n%s:%u in %s\n", msg, file, line, func);
    __builtin_trap();
}

void check_wasm_key() {
    if (wasm_pending_key != 0) {
        int key = wasm_pending_key;
        wasm_pending_key = 0;
        Keyboard::the().handle_scancode((byte)key);
    }
}
