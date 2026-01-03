#include "Size.h"
#include <AK/Types.h>
#include <Terminal.h>
#include <Scheduler.h>
#include <Process.h>
#include "kprintf.h"
#include "system.h"
#include <SharedGraphics/GraphicsBitmap.h>
#include <Kernel/CpioFileSystem.h>
#include "Painter.h"
#include "Syscall.h"
#include "Keyboard.h"

static uint32_t user_stack[8192];
static uint32_t kernel_stack[8192];

extern void shell_main();
Terminal* terminal1 = nullptr;
Terminal* terminal2 = nullptr;
system_t system;

void shell_entry() {
// #ifdef I386    
//     enter_user_mode(
//         (uint32_t)&shell_main,                  // Entry point
//         (uint32_t)user_stack + sizeof(user_stack), // User ESP
//         (uint32_t)kernel_stack + sizeof(kernel_stack) // Kernel ESP0 (for handling interrupts later)
//     );
// #else
//     shell_main();    
// #endif    
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
extern "C" void kernel_entry(int width, int height, uint32_t framebuffer, uint8_t* cpio_start) 
{
    Size size(width, height);
    RGBA32* fb_ptr = (RGBA32*)(framebuffer);

    dbgprintf("kernel_entry: fb_ptr = %p\n", fb_ptr);

    keyboard = new Keyboard;
    Process::initialize();
    Syscall::initialize();

    terminal2 = new Terminal({width/2,height/2});
    terminal1 = new Terminal({0,0});
    terminal1->create_window(size, fb_ptr);
    terminal2->create_window(size, fb_ptr);

    Process::create_kernel_process("Shell", shell_entry);
    Process::create_kernel_process("Background", task2_entry);

    RetainPtr<GraphicsBitmap> backing = GraphicsBitmap::create_wrapper(size, fb_ptr);
    Rect rect { 0, 0, width, height };
    Painter painter(*backing);
    painter.fill_rect(rect, Color::Black);
    terminal1->paint();
    terminal2->paint();

    CpioFileSystem::initialize(cpio_start);
    Process::create_user_process("bin/clear");
}