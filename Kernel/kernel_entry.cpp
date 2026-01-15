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

extern void shell_main();
Terminal* terminal1 = nullptr;
Terminal* terminal2 = nullptr;
system_t system;

void task1_entry() {
    while (true) {
        Scheduler::yield(); 
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

    Process::create_kernel_process("Shell", task1_entry);
    Process::create_kernel_process("Background", task2_entry);

    RetainPtr<GraphicsBitmap> backing = GraphicsBitmap::create_wrapper(size, fb_ptr);
    Rect rect { 0, 0, width, height };
    Painter painter(*backing);
    painter.fill_rect(rect, Color::Black);
    terminal1->paint();
    terminal2->paint();

    CpioFileSystem::initialize(cpio_start);
    Process::create_user_process("bin/sh");
}