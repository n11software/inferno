#ifndef VGA_FONT_H
#define VGA_FONT_H

#include <Inferno/stdint.h>

// Convert ASCII to font index for all supported characters
#define ASCII_TO_FONT(c) \
    ((c >= 'a' && c <= 'z') ? (c - 'a' + 26) : \
     (c >= 'A' && c <= 'Z') ? (c - 'A') : \
     (c >= '0' && c <= '9') ? (c - '0' + 52) : \
     (c == '!') ? 62 : \
     (c == '@') ? 63 : \
     (c == '#') ? 64 : \
     (c == '$') ? 65 : \
     (c == '%') ? 66 : \
     (c == '^') ? 67 : \
     (c == '&') ? 68 : \
     (c == '*') ? 69 : \
     (c == '(') ? 70 : \
     (c == ')') ? 71 : \
     (c == '[') ? 72 : \
     (c == ']') ? 73 : \
     (c == '-') ? 74 : \
     (c == '+') ? 75 : \
     (c == '=') ? 76 : \
     (c == '/') ? 77 : \
     (c == '\\') ? 78 : \
     (c == ',') ? 79 : \
     (c == '.') ? 80 : \
     (c == '<') ? 81 : \
     (c == '>') ? 82 : \
     (c == '?') ? 83 : \
     (c == ' ') ? 84 : \
     (c == ':') ? 85 : \
     (c == ';') ? 86 : \
     (c == '\'') ? 87 : \
     (c == '"') ? 88 : \
     (c == '{') ? 89 : \
     (c == '}') ? 90 : \
     (c == '~') ? 91 : \
     (c == '`') ? 92 : 0)

static const uint8_t font8x8[93][8] = {
    // Uppercase letters (0-25)
    {0x18, 0x24, 0x42, 0x7E, 0x42, 0x42, 0x42, 0x00}, // A
    {0x7C, 0x42, 0x42, 0x7C, 0x42, 0x42, 0x7C, 0x00}, // B
    {0x3C, 0x42, 0x40, 0x40, 0x40, 0x42, 0x3C, 0x00}, // C
    {0x78, 0x44, 0x42, 0x42, 0x42, 0x44, 0x78, 0x00}, // D
    {0x7E, 0x40, 0x40, 0x7C, 0x40, 0x40, 0x7E, 0x00}, // E
    {0x7E, 0x40, 0x40, 0x7C, 0x40, 0x40, 0x40, 0x00}, // F
    {0x3C, 0x42, 0x40, 0x4E, 0x42, 0x42, 0x3C, 0x00}, // G
    {0x42, 0x42, 0x42, 0x7E, 0x42, 0x42, 0x42, 0x00}, // H
    {0x3E, 0x08, 0x08, 0x08, 0x08, 0x08, 0x3E, 0x00}, // I
    {0x1F, 0x04, 0x04, 0x04, 0x44, 0x44, 0x38, 0x00}, // J
    {0x42, 0x44, 0x48, 0x70, 0x48, 0x44, 0x42, 0x00}, // K
    {0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x7E, 0x00}, // L
    {0x42, 0x66, 0x5A, 0x42, 0x42, 0x42, 0x42, 0x00}, // M
    {0x42, 0x62, 0x52, 0x4A, 0x46, 0x42, 0x42, 0x00}, // N
    {0x3C, 0x42, 0x42, 0x42, 0x42, 0x42, 0x3C, 0x00}, // O
    {0x7C, 0x42, 0x42, 0x7C, 0x40, 0x40, 0x40, 0x00}, // P
    {0x3C, 0x42, 0x42, 0x42, 0x4A, 0x44, 0x3A, 0x00}, // Q
    {0x7C, 0x42, 0x42, 0x7C, 0x48, 0x44, 0x42, 0x00}, // R
    {0x3C, 0x42, 0x40, 0x3C, 0x02, 0x42, 0x3C, 0x00}, // S
    {0x7F, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00}, // T
    {0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x3C, 0x00}, // U
    {0x42, 0x42, 0x42, 0x42, 0x24, 0x24, 0x18, 0x00}, // V
    {0x42, 0x42, 0x42, 0x42, 0x5A, 0x66, 0x42, 0x00}, // W
    {0x42, 0x24, 0x18, 0x18, 0x18, 0x24, 0x42, 0x00}, // X
    {0x41, 0x22, 0x14, 0x08, 0x08, 0x08, 0x08, 0x00}, // Y
    {0x7E, 0x02, 0x04, 0x08, 0x10, 0x20, 0x7E, 0x00}, // Z

    // Lowercase letters (26-51)
    {0x00, 0x00, 0x3C, 0x02, 0x3E, 0x42, 0x3E, 0x00}, // a
    {0x40, 0x40, 0x5C, 0x62, 0x42, 0x42, 0x3C, 0x00}, // b
    {0x00, 0x00, 0x3C, 0x42, 0x40, 0x42, 0x3C, 0x00}, // c
    {0x02, 0x02, 0x3A, 0x46, 0x42, 0x42, 0x3E, 0x00}, // d
    {0x00, 0x00, 0x3C, 0x42, 0x7E, 0x40, 0x3C, 0x00}, // e
    {0x0C, 0x12, 0x10, 0x7C, 0x10, 0x10, 0x10, 0x00}, // f
    {0x00, 0x00, 0x3E, 0x42, 0x42, 0x3E, 0x02, 0x3C}, // g
    {0x40, 0x40, 0x5C, 0x62, 0x42, 0x42, 0x42, 0x00}, // h
    {0x08, 0x00, 0x18, 0x08, 0x08, 0x08, 0x1C, 0x00}, // i
    {0x04, 0x00, 0x0C, 0x04, 0x04, 0x44, 0x38, 0x00}, // j
    {0x40, 0x40, 0x44, 0x48, 0x70, 0x48, 0x44, 0x00}, // k
    {0x18, 0x08, 0x08, 0x08, 0x08, 0x08, 0x1C, 0x00}, // l
    {0x00, 0x00, 0x76, 0x49, 0x49, 0x49, 0x49, 0x00}, // m
    {0x00, 0x00, 0x5C, 0x62, 0x42, 0x42, 0x42, 0x00}, // n
    {0x00, 0x00, 0x3C, 0x42, 0x42, 0x42, 0x3C, 0x00}, // o
    {0x00, 0x00, 0x7C, 0x42, 0x42, 0x7C, 0x40, 0x40}, // p
    {0x00, 0x00, 0x3E, 0x42, 0x42, 0x3E, 0x02, 0x02}, // q
    {0x00, 0x00, 0x5C, 0x62, 0x40, 0x40, 0x40, 0x00}, // r
    {0x00, 0x00, 0x3E, 0x40, 0x3C, 0x02, 0x7C, 0x00}, // s
    {0x10, 0x10, 0x7C, 0x10, 0x10, 0x12, 0x0C, 0x00}, // t
    {0x00, 0x00, 0x42, 0x42, 0x42, 0x46, 0x3A, 0x00}, // u
    {0x00, 0x00, 0x42, 0x42, 0x42, 0x24, 0x18, 0x00}, // v
    {0x00, 0x00, 0x41, 0x49, 0x49, 0x49, 0x36, 0x00}, // w
    {0x00, 0x00, 0x42, 0x24, 0x18, 0x24, 0x42, 0x00}, // x
    {0x00, 0x00, 0x42, 0x42, 0x42, 0x3E, 0x02, 0x3C}, // y
    {0x00, 0x00, 0x7E, 0x04, 0x08, 0x10, 0x7E, 0x00}, // z

    // Numbers (52-61)
    {0x3C, 0x42, 0x46, 0x4A, 0x52, 0x62, 0x3C, 0x00}, // 0
    {0x08, 0x18, 0x28, 0x08, 0x08, 0x08, 0x3E, 0x00}, // 1
    {0x3C, 0x42, 0x02, 0x0C, 0x30, 0x40, 0x7E, 0x00}, // 2
    {0x3C, 0x42, 0x02, 0x1C, 0x02, 0x42, 0x3C, 0x00}, // 3
    {0x04, 0x0C, 0x14, 0x24, 0x7E, 0x04, 0x04, 0x00}, // 4
    {0x7E, 0x40, 0x7C, 0x02, 0x02, 0x42, 0x3C, 0x00}, // 5
    {0x1C, 0x20, 0x40, 0x7C, 0x42, 0x42, 0x3C, 0x00}, // 6
    {0x7E, 0x02, 0x04, 0x08, 0x10, 0x20, 0x20, 0x00}, // 7
    {0x3C, 0x42, 0x42, 0x3C, 0x42, 0x42, 0x3C, 0x00}, // 8
    {0x3C, 0x42, 0x42, 0x3E, 0x02, 0x04, 0x38, 0x00}, // 9

    // Special characters (62-84)
    {0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x08, 0x00}, // !
    {0x3C, 0x42, 0x4A, 0x5C, 0x4A, 0x40, 0x3C, 0x00}, // @
    {0x24, 0x24, 0x7E, 0x24, 0x7E, 0x24, 0x24, 0x00}, // #
    {0x08, 0x3E, 0x48, 0x3E, 0x0A, 0x3E, 0x08, 0x00}, // $
    {0x62, 0x64, 0x08, 0x10, 0x20, 0x46, 0x86, 0x00}, // %
    {0x10, 0x28, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00}, // ^
    {0x38, 0x44, 0x44, 0x38, 0x44, 0x44, 0x3A, 0x00}, // &
    {0x14, 0x14, 0x14, 0x14, 0x14, 0x00, 0x00, 0x00}, // *
    {0x08, 0x10, 0x20, 0x20, 0x20, 0x10, 0x08, 0x00}, // (
    {0x20, 0x10, 0x08, 0x08, 0x08, 0x10, 0x20, 0x00}, // )
    {0x1E, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1E, 0x00}, // [ (wider left bracket)
    {0x78, 0x08, 0x08, 0x08, 0x08, 0x08, 0x78, 0x00}, // ] (wider right bracket)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // space (fixed - completely empty)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x00}, // , (fixed - proper comma shape)

    // Add new characters (indices 85-92)
    {0x00, 0x08, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00}, // : (made more visible with 2x2 dots)
    {0x00, 0x08, 0x00, 0x00, 0x08, 0x08, 0x10, 0x00}, // ; (86)
    {0x00, 0x08, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00}, // ' (87)
    {0x00, 0x24, 0x48, 0x00, 0x00, 0x00, 0x00, 0x00}, // " (88)
    {0x0C, 0x10, 0x10, 0x20, 0x10, 0x10, 0x0C, 0x00}, // { (89)
    {0x30, 0x08, 0x08, 0x04, 0x08, 0x08, 0x30, 0x00}, // } (90)
    {0x00, 0x24, 0x48, 0x00, 0x00, 0x00, 0x00, 0x00}, // ~ (91)
    {0x00, 0x08, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00}  // ` (92)
};

#endif /* VGA_FONT_H */