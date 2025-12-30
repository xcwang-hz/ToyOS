#pragma once

#include "Terminal.h"
#ifdef WASM
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
// Static buffer for Wasm
extern uint32_t wasm_framebuffer[];
extern "C" void canvas_init(uint32_t* ptr, int width, int height);
void check_js_key();
#endif

extern Terminal* terminal1;
extern Terminal* terminal2;