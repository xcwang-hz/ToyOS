typedef unsigned short word;
typedef unsigned char byte;

typedef word uint16_t;
typedef byte uint8_t;

// VGA Text Mode Buffer location
// 0xB8000 is the standard memory address for color text screens
static volatile uint16_t* vga_buffer = (uint16_t*)0xB8000;

// Default color: Light Grey text on Black background (0x07)
static const uint8_t DEFAULT_COLOR = 0x07;

extern "C" void host_console_log(const char* ptr) {
    int i = 0;
    while (ptr[i] != '\0') {
        // VGA format: High byte is color, Low byte is character
        vga_buffer[i] = (uint16_t)DEFAULT_COLOR << 8 | (uint16_t)ptr[i];
        i++;
    }
}