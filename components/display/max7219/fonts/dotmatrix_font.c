#include "dotmatrix_font.h"
#include "default_font.h"
#include <stddef.h>

/**
 * @brief Dot matrix style font data (5x8 with top line "roof")
 *
 * HOW THIS FONT WORKS:
 * ====================
 *
 * Each character has a distinctive "roof" (top line) that spans the full width.
 * This creates a classic dot matrix LED display look.
 *
 * Character format: 5 columns wide, 8 rows tall
 * Bit 0 (LSB) = Top pixel (the "roof")
 * Bit 1       = Empty space between roof and letter
 * Bits 2-7    = Letter shape (6 pixels tall, fills to BOTTOM)
 *
 * Example: Letter 'A'
 *   Visual (8x5):      Column bytes (binary):
 *   X X X X X          [0b11111101 = 0xFD,  (roof at top)
 *   . . . . .           0b10010001 = 0x91,  (space)
 *   X . . . X           0b10010001 = 0x91,  (letter body)
 *   X . . . X           0b10010001 = 0x91,  (letter body)
 *   X X X X X           0b10010001 = 0x91,  (letter body)
 *   X . . . X           0b10110001 = 0xB1,  (letter body)
 *   X . . . X           0b11111101 = 0xFD]  (letter at bottom)
 *   X . . . X
 */

// Font data for letters A-Z and a-z (lowercase same as uppercase)
// Bit 0: Roof (always 1)
// Bit 1: Empty space (always 0)
// Bits 2-7: Letter shape (6 pixels tall, anchored to bottom = bit 7)
// Each letter: 5 columns (spacing with roof will be added by get_char)
static const uint8_t DOTMATRIX_FONT_DATA[] = {
    // A (offset 0) - Triangle with horizontal bar
    0xFD, 0x95, 0x93, 0x95, 0xFD,

    // B (offset 5) - Vertical bar with two bumps
    0xFD, 0x95, 0x95, 0x95, 0x69,

    // C (offset 10) - Curved bracket (symmetric)
    0x79, 0x85, 0x85, 0x85, 0x85,

    // D (offset 15) - Vertical bar with curve
    0xFD, 0x85, 0x85, 0x85, 0x79,

    // E (offset 20) - Three horizontal bars
    0xFD, 0x95, 0x95, 0x95, 0x85,

    // F (offset 25) - Two horizontal bars (top and middle)
    0xFD, 0x15, 0x15, 0x15, 0x05,

    // G (offset 30) - C with inner bars
    0x79, 0x85, 0x95, 0x95, 0xF5,

    // H (offset 35) - Two verticals with middle bar
    0xFD, 0x11, 0x11, 0x11, 0xFD,

    // I (offset 40) - Center vertical with bars
    0x85, 0x85, 0xFD, 0x85, 0x85,

    // J (offset 45) - Hook shape
    0x41, 0x81, 0x85, 0x7D, 0x05,

    // K (offset 50) - Vertical with angled lines
    0xFD, 0x11, 0x29, 0x45, 0x85,

    // L (offset 55) - Vertical with bottom bar
    0xFD, 0x81, 0x81, 0x81, 0x81,

    // M (offset 60) - Two verticals with peak
    0xFD, 0x05, 0x19, 0x05, 0xFD,

    // N (offset 65) - Two verticals with diagonal
    0xFD, 0x05, 0x11, 0x41, 0xFD,

    // O (offset 70) - Rectangle (symmetric)
    0x79, 0x85, 0x85, 0x85, 0x79,

    // P (offset 75) - Vertical with top bump
    0xFD, 0x15, 0x15, 0x15, 0x09,

    // Q (offset 80) - O with tail
    0x79, 0x85, 0xA5, 0xC5, 0xF9,

    // R (offset 85) - P with leg
    0xFD, 0x15, 0x35, 0x55, 0x89,

    // S (offset 90) - Snake/zigzag
    0x49, 0x95, 0x95, 0x95, 0x65,

    // T (offset 95) - Top bar with center vertical
    0x05, 0x05, 0xFD, 0x05, 0x05,

    // U (offset 100) - Two verticals with bottom
    0x7D, 0x81, 0x81, 0x81, 0x7D,

    // V (offset 105) - Two diagonals meeting at bottom
    0x3D, 0x41, 0x81, 0x41, 0x3D,

    // W (offset 110) - W shape
    0x7D, 0x81, 0x7D, 0x81, 0x7D,

    // X (offset 115) - Two diagonals crossing
    0xC5, 0x29, 0x11, 0x29, 0xC5,

    // Y (offset 120) - Two diagonals to center vertical
    0x05, 0x11, 0xE5, 0x11, 0x05,

    // Z (offset 125) - Diagonal with bars
    0xC5, 0xA5, 0x95, 0x8D, 0x85,

    // ° (offset 130) - Degree symbol (small circle at top)
    0x0F, 0x09, 0x0F, 0x00, 0x00,
};

// Character widths (all letters are 5 columns, spacing will be added by get_char)
static const uint8_t DOTMATRIX_CHAR_WIDTH[256] = {
    // Control chars (0-31): width 0
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    // Space and symbols (32-64): width 0 (will use default font)
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,

    // A-Z (65-90): width 5 (letter only, spacing added by get_char)
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5,

    // Symbols (91-96): width 0 (will use default font)
    0, 0, 0, 0, 0, 0,

    // a-z (97-122): width 5 (same as uppercase)
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5,

    // Rest (123-175): width 0 (will use default font)
    [123 ... 175] = 0,

    // ° (176/0xB0): degree symbol, width 5
    5,

    // Rest (177-255): width 0 (will use default font)
    [177 ... 255] = 0,
};

// Character offsets in DOTMATRIX_FONT_DATA (now 5 bytes per letter)
static const uint16_t DOTMATRIX_CHAR_OFFSET[256] = {
    // Control chars (0-31): offset 0
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    // Space and symbols (32-64): offset 0 (will use default font)
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,

    // A-Z (65-90) - offsets in steps of 5
    0,   // A
    5,   // B
    10,  // C
    15,  // D
    20,  // E
    25,  // F
    30,  // G
    35,  // H
    40,  // I
    45,  // J
    50,  // K
    55,  // L
    60,  // M
    65,  // N
    70,  // O
    75,  // P
    80,  // Q
    85,  // R
    90,  // S
    95,  // T
    100, // U
    105, // V
    110, // W
    115, // X
    120, // Y
    125, // Z

    // Symbols (91-96): offset 0 (will use default font)
    0, 0, 0, 0, 0, 0,

    // a-z (97-122): same as A-Z
    0,   // a -> A
    5,   // b -> B
    10,  // c -> C
    15,  // d -> D
    20,  // e -> E
    25,  // f -> F
    30,  // g -> G
    35,  // h -> H
    40,  // i -> I
    45,  // j -> J
    50,  // k -> K
    55,  // l -> L
    60,  // m -> M
    65,  // n -> N
    70,  // o -> O
    75,  // p -> P
    80,  // q -> Q
    85,  // r -> R
    90,  // s -> S
    95,  // t -> T
    100, // u -> U
    105, // v -> V
    110, // w -> W
    115, // x -> X
    120, // y -> Y
    125, // z -> Z

    // Rest (123-175): offset 0 (will use default font)
    [123 ... 175] = 0,

    // ° (176/0xB0): degree symbol
    130,

    // Rest (177-255): offset 0 (will use default font)
    [177 ... 255] = 0,
};

// Character descriptors (reused for performance)
static font_char_t s_dotmatrix_char;
static font_char_t s_dotmatrix_char_last;

const font_char_t* dotmatrix_font_get_char(uint8_t ch)
{
    static uint8_t char_data_with_spacing[6];  // 5 letter + 1 spacing with roof

    uint8_t base_width = DOTMATRIX_CHAR_WIDTH[ch];

    // If width is 0, this character is not in our font - use default
    if (base_width == 0) {
        return default_font_get_char(ch);
    }

    // Copy character data and add spacing column with roof
    uint16_t offset = DOTMATRIX_CHAR_OFFSET[ch];
    const uint8_t *base_data = &DOTMATRIX_FONT_DATA[offset];

    for (int i = 0; i < base_width; i++) {
        char_data_with_spacing[i] = base_data[i];
    }
    char_data_with_spacing[base_width] = 0x01;  // Add roof pixel for continuous techito

    // Return character with spacing included
    s_dotmatrix_char.width = base_width + 1;
    s_dotmatrix_char.data = char_data_with_spacing;

    return &s_dotmatrix_char;
}

const font_char_t* dotmatrix_font_get_char_last(uint8_t ch)
{
    uint8_t base_width = DOTMATRIX_CHAR_WIDTH[ch];

    // If width is 0, this character is not in our font - use default's get_char_last
    if (base_width == 0) {
        return font_default.get_char_last(ch);
    }

    // Return character WITHOUT spacing column (no techito extension)
    uint16_t offset = DOTMATRIX_CHAR_OFFSET[ch];
    s_dotmatrix_char_last.data = &DOTMATRIX_FONT_DATA[offset];
    s_dotmatrix_char_last.width = base_width;

    return &s_dotmatrix_char_last;
}

// Font descriptor for the dot matrix font
const font_t font_dotmatrix = {
    .name = "dotmatrix",
    .get_char = dotmatrix_font_get_char,
    .get_char_last = dotmatrix_font_get_char_last
};
