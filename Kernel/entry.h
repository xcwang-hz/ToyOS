#pragma once

#include <Kernel/Terminal/Terminal.h>
extern Terminal* terminal1;
extern Terminal* terminal2;
extern "C" void kernel_entry(int width, int height, uint32_t framebuffer, uint8_t* cpio_start);