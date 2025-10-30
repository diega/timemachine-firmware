#include "dotmatrix_small_font.h"
#include "default_font.h"
#include <stddef.h>

/**
 * @brief Small dot matrix style font data (3x8 with top line "roof")
 *
 * Each character: 3 columns (reduced from 5)
 * Bit 0 (LSB) = Top pixel (the "roof")
 * Bit 1       = Empty space between roof and letter
 * Bits 2-7    = Letter shape (6 pixels tall, fills to BOTTOM)
 */

// Font data for letters A-Z and a-z (lowercase same as uppercase)
// Each letter: 3 columns (spacing with roof will be added by get_char)
static const uint8_t DOTMATRIX_SMALL_FONT_DATA[] = {
    // A (offset 0) - Triangle with bar
    0xFD, 0x15, 0xFD,

    // B (offset 3) - Vertical with bumps
    0xFD, 0x95, 0x6D,

    // C (offset 6) - Curve
    0x7D, 0x85, 0x85,

    // D (offset 9) - Vertical with curve
    0xFD, 0x85, 0x7D,

    // E (offset 12) - Three bars
    0xFD, 0x95, 0x85,

    // F (offset 15) - Two bars (top and middle)
    0xFD, 0x15, 0x05,

    // G (offset 18) - C with inner bar
    0x7D, 0x95, 0xB5,

    // H (offset 21) - Two verticals with middle
    0xFD, 0x11, 0xFD,

    // I (offset 24) - Center vertical
    0x85, 0xFD, 0x85,

    // J (offset 27) - Hook
    0x81, 0x85, 0x7D,

    // K (offset 30) - Vertical with angles
    0xFD, 0x11, 0xED,

    // L (offset 33) - Vertical with bottom
    0xFD, 0x81, 0x81,

    // M (offset 36) - Peak
    0xFD, 0x0D, 0xFD,

    // N (offset 39) - Diagonal
    0xFD, 0x11, 0xFD,

    // O (offset 42) - Rectangle
    0x7D, 0x85, 0x7D,

    // P (offset 45) - Top bump
    0xFD, 0x15, 0x0D,

    // Q (offset 48) - O with tail
    0x7D, 0xA5, 0xFD,

    // R (offset 51) - P with leg
    0xFD, 0x15, 0xED,

    // S (offset 54) - Snake
    0x4D, 0x95, 0xB5,

    // T (offset 57) - Top bar with vertical
    0x05, 0xFD, 0x05,

    // U (offset 60) - Two verticals with bottom
    0x7D, 0x81, 0x7D,

    // V (offset 63) - Diagonals to bottom
    0x3D, 0xC1, 0x3D,

    // W (offset 66) - W shape
    0xFD, 0x61, 0xFD,

    // X (offset 69) - Cross
    0xED, 0x11, 0xED,

    // Y (offset 72) - Y shape
    0x0D, 0xF1, 0x0D,

    // Z (offset 75) - Diagonal with bars
    0xE5, 0x95, 0x8D,
};

// Character widths (all letters are 3 columns, spacing will be added by get_char)
static const uint8_t DOTMATRIX_SMALL_CHAR_WIDTH[256] = {
    // Control chars (0-31): width 0
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    // Space and symbols (32-64): width 0 (will use default font)
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,

    // A-Z (65-90): width 3 (letter only, spacing added by get_char)
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3,

    // Symbols (91-96): width 0 (will use default font)
    0, 0, 0, 0, 0, 0,

    // a-z (97-122): width 3 (same as uppercase)
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3,

    // Rest (123-255): 133 chars, width 0 (will use default font)
    [123 ... 255] = 0,
};

// Character offsets in DOTMATRIX_SMALL_FONT_DATA (now 3 bytes per letter)
static const uint16_t DOTMATRIX_SMALL_CHAR_OFFSET[256] = {
    // Control chars (0-31): offset 0
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    // Space and symbols (32-64): offset 0 (will use default font)
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,

    // A-Z (65-90) - offsets in steps of 3
    0,   // A
    3,   // B
    6,   // C
    9,   // D
    12,  // E
    15,  // F
    18,  // G
    21,  // H
    24,  // I
    27,  // J
    30,  // K
    33,  // L
    36,  // M
    39,  // N
    42,  // O
    45,  // P
    48,  // Q
    51,  // R
    54,  // S
    57,  // T
    60,  // U
    63,  // V
    66,  // W
    69,  // X
    72,  // Y
    75,  // Z

    // Symbols (91-96): offset 0 (will use default font)
    0, 0, 0, 0, 0, 0,

    // a-z (97-122): same as A-Z
    0,   // a -> A
    3,   // b -> B
    6,   // c -> C
    9,   // d -> D
    12,  // e -> E
    15,  // f -> F
    18,  // g -> G
    21,  // h -> H
    24,  // i -> I
    27,  // j -> J
    30,  // k -> K
    33,  // l -> L
    36,  // m -> M
    39,  // n -> N
    42,  // o -> O
    45,  // p -> P
    48,  // q -> Q
    51,  // r -> R
    54,  // s -> S
    57,  // t -> T
    60,  // u -> U
    63,  // v -> V
    66,  // w -> W
    69,  // x -> X
    72,  // y -> Y
    75,  // z -> Z

    // Rest (123-255): 133 chars, offset 0 (will use default font)
    [123 ... 255] = 0,
};

// Character descriptors (reused for performance)
static font_char_t s_dotmatrix_small_char;
static font_char_t s_dotmatrix_small_char_last;

const font_char_t* dotmatrix_small_font_get_char(uint8_t ch)
{
    static uint8_t char_data_with_spacing[4];  // 3 letter + 1 spacing with roof

    uint8_t base_width = DOTMATRIX_SMALL_CHAR_WIDTH[ch];

    // If width is 0, this character is not in our font - use default
    if (base_width == 0) {
        return default_font_get_char(ch);
    }

    // Copy character data and add spacing column with roof
    uint16_t offset = DOTMATRIX_SMALL_CHAR_OFFSET[ch];
    const uint8_t *base_data = &DOTMATRIX_SMALL_FONT_DATA[offset];

    for (int i = 0; i < base_width; i++) {
        char_data_with_spacing[i] = base_data[i];
    }
    char_data_with_spacing[base_width] = 0x01;  // Add roof pixel for continuous techito

    // Return character with spacing included
    s_dotmatrix_small_char.width = base_width + 1;
    s_dotmatrix_small_char.data = char_data_with_spacing;

    return &s_dotmatrix_small_char;
}

const font_char_t* dotmatrix_small_font_get_char_last(uint8_t ch)
{
    uint8_t base_width = DOTMATRIX_SMALL_CHAR_WIDTH[ch];

    // If width is 0, this character is not in our font - use default's get_char_last
    if (base_width == 0) {
        return font_default.get_char_last(ch);
    }

    // Return character WITHOUT spacing column (no techito extension)
    uint16_t offset = DOTMATRIX_SMALL_CHAR_OFFSET[ch];
    s_dotmatrix_small_char_last.data = &DOTMATRIX_SMALL_FONT_DATA[offset];
    s_dotmatrix_small_char_last.width = base_width;

    return &s_dotmatrix_small_char_last;
}

// Font descriptor for the small dot matrix font
const font_t font_dotmatrix_small = {
    .name = "dotmatrix_small",
    .get_char = dotmatrix_small_font_get_char,
    .get_char_last = dotmatrix_small_font_get_char_last
};
