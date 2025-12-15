#pragma once

#include <AK/Types.h>

typedef dword RGBA32;

#ifdef WASM
    // [Wasm / HTML5 Canvas]
    // In memory: R G B A (Little Endian)
    // HEX: 0xAABBGGRR
    inline constexpr dword make_rgb(byte r, byte g, byte b)
    {
        return 0xFF000000 | (b << 16) | (g << 8) | r;
    }
#else
    // In memory: B G R A (or B G R 0)
    // HEX: 0x00RRGGBB
    inline constexpr dword make_rgb(byte r, byte g, byte b)
    {
        return ((r << 16) | (g << 8) | b);
    }
#endif

class Color {
public:
    enum NamedColor {
        Black,
        White,
        Red,
        Green,
        Blue,
        Yellow,
        Magenta,
        DarkGray,
        MidGray,
        LightGray,
    };

    Color() { }
    Color(NamedColor);
    Color(byte r, byte g, byte b) : m_value(make_rgb(r, g, b)) { }
    Color(RGBA32 rgba) : m_value(rgba) { }

    int red() const { 
        #ifdef WASM
            return m_value & 0xff;
        #else
            return (m_value >> 16) & 0xff;
        #endif
    }
    int green() const { return (m_value >> 8) & 0xff; }
    int blue() const { 
        #ifdef WASM
            return (m_value >> 16) & 0xff;
        #else
            return m_value & 0xff;
        #endif
    }

    RGBA32 value() const { return m_value; }

private:
    RGBA32 m_value { 0 };
};
