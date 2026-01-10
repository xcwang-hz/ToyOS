#pragma once
#include <AK/Types.h>

extern "C" void js_context_switch(void* prev_ctx_addr, void* next_ctx_addr);
extern "C" void js_canvas_init(uint32_t* ptr, int width, int height);
extern "C" void js_console_error(const char* msg);        // directly report to console log
extern "C" void js_debug_char(char c);                  // called by dbgprintf/kprintf
extern "C" int js_load_user_process(const uint8_t* ptr, uint32_t size);
extern "C" bool js_start_user_process(int32_t user_proc_id);
void check_wasm_key();
