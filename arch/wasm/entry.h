#pragma once
#include <AK/Types.h>

extern "C" void js_context_switch(void* prev_ctx_addr, void* next_ctx_addr);
extern "C" void js_canvas_init(uint32_t* ptr, int width, int height);
extern "C" void js_debug_char(char c);
extern "C" void js_spawn_user_process(const uint8_t* ptr, uint32_t size);
void check_wasm_key();
