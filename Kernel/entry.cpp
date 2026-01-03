#include "Size.h"
#include <AK/Types.h>
#include <Terminal.h>
#include <Scheduler.h>
#include <Process.h>
#include "kprintf.h"
#include "system.h"
#include <SharedGraphics/GraphicsBitmap.h>
#include "Painter.h"
#include "Syscall.h"
#include "Keyboard.h"

struct CPIOHeader {
    char c_magic[6];    // "070701"
    char c_ino[8];
    char c_mode[8];
    char c_uid[8];
    char c_gid[8];
    char c_nlink[8];
    char c_mtime[8];
    char c_filesize[8];
    char c_devmajor[8];
    char c_devminor[8];
    char c_rdevmajor[8];
    char c_rdevminor[8];
    char c_namesize[8];
    char c_check[8];
};

static uint32_t user_stack[8192];
static uint32_t kernel_stack[8192];

extern void shell_main();
Terminal* terminal1 = nullptr;
Terminal* terminal2 = nullptr;
system_t system;

static uint8_t* cpio_archive = nullptr;
inline uint32_t hex2int(const char* s) {
    uint32_t val = 0;
    for (int i = 0; i < 8; i++) {
        val <<= 4;
        if (s[i] >= '0' && s[i] <= '9') val += s[i] - '0';
        else if (s[i] >= 'a' && s[i] <= 'f') val += s[i] - 'a' + 10;
        else if (s[i] >= 'A' && s[i] <= 'F') val += s[i] - 'A' + 10;
    }
    return val;
}

uint8_t* cpio_find_file(const char* target_name, uint32_t* out_size) {
    if (!cpio_archive) 
        return nullptr;

    uint8_t* current = cpio_archive;
    while (true) {
        auto* header = (CPIOHeader*)current;

        // 1. Check Magic
        if (strncmp(header->c_magic, "070701", 6) != 0) return nullptr;

        uint32_t namesize = hex2int(header->c_namesize);
        uint32_t filesize = hex2int(header->c_filesize);

        // 2. Get current filename
        char* filename = (char*)(current + sizeof(CPIOHeader));

        // 3. Check for end of archive
        if (strcmp(filename, "TRAILER!!!") == 0) return nullptr;

        // 4. Calculate alignment (CPIO Data is 4-byte aligned)
        uint32_t head_len = sizeof(CPIOHeader) + namesize;
        if (head_len % 4 != 0) head_len += 4 - (head_len % 4);
        
        uint8_t* file_data = current + head_len;

        // 5. [Key] Compare filenames
        if (strcmp(filename, target_name) == 0 || 
           (strlen(filename) > strlen(target_name) && 
            strcmp(filename + strlen(filename) - strlen(target_name), target_name) == 0)) 
        {
            if (out_size) *out_size = filesize;
            return file_data;
        }

        // 6. Jump to next file
        uint32_t next_offset = head_len + filesize;
        if (next_offset % 4 != 0) next_offset += 4 - (next_offset % 4);
        current += next_offset;
    }
}

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
    cpio_archive = cpio_start;

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
}