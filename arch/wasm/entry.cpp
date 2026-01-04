#include <AK/Types.h>
#include <Kernel/kprintf.h>
#include <Kernel/entry.h>
#include <Kernel/Scheduler.h>
#include <Kernel/Process.h>
#include <arch/wasm/entry.h>
#include <Kernel/Syscall.h>

bool initialized = false;
const int SCREEN_WIDTH = 1024;
const int SCREEN_HEIGHT = 768;
uint32_t wasm_framebuffer[SCREEN_WIDTH * SCREEN_HEIGHT];

struct SyscallParams {
    dword func;
    dword arg1;
    dword arg2;
    dword arg3;
    dword retval;
};

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
        if (current->m_entry) {
            current->m_entry();
            current->m_is_first_time = false;
        } 
        else if (current->m_user_proc_id > 0) {
            if (js_start_user_process(current->m_user_proc_id))
                current->m_is_first_time = false;
            else
                Scheduler::yield();
        }
    }
    Scheduler::timer_tick();    
}

extern "C" dword wasm_syscall_handle(uint32_t buffer_addr)
{
    SyscallParams* params = (SyscallParams*)buffer_addr;
    dword result = js_syscall_handle(params->func, params->arg1, params->arg2, params->arg3);
    params->retval = result;
    return result;
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
