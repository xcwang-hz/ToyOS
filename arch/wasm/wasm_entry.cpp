#include "Keyboard.h"

extern "C" {
    void wasm_send_key(int scancode) {
        Keyboard::the().handle_scancode((byte)scancode);
    }
}