#include "default_font.h"
#include <stddef.h>

/**
 * @brief Default font data adapted from ESPTimeCast
 *
 * Original source: https://github.com/mfactory-osaka/ESPTimeCast
 *
 * HOW THIS FONT WORKS:
 * ====================
 *
 * The MAX7219 LED matrix displays are organized in COLUMNS of 8 pixels each.
 * Each character is represented as a series of vertical columns.
 *
 * COLUMN BYTE FORMAT:
 * -------------------
 * Each byte represents one vertical column (8 pixels high):
 *   Bit 0 (LSB) = Top pixel
 *   Bit 1       = 2nd pixel from top
 *   Bit 2       = 3rd pixel from top
 *   ...
 *   Bit 7 (MSB) = Bottom pixel
 *
 * Example: The digit '1'
 *   Visual (8x3):  Column bytes (decimal):
 *   . X .          [130,        (0b10000010)
 *   X X .           255,        (0b11111111)
 *   . X .           128]        (0b10000000)
 *   . X .
 *   . X .          Width = 3 (three columns)
 *   . X .
 *   . X .          Rendered left-to-right:
 *   . X .          Col0  Col1  Col2
 *
 * In this file, the digit '1' at ASCII 49 is stored as:
 *   - Data: [130, 255, 128] (these are the decimal values of the column bytes)
 *   - Width: 3 (the character uses 3 columns)
 *   - Offset: 147 (where in DEFAULT_FONT_FONT_DATA this character's data starts)
 *
 * Another example: The digit '0' (rounded rectangle)
 *   Visual (8x3):  Column bytes (decimal):
 *   . X .          [126,        (0b01111110)
 *   X . X           129,        (0b10000001)
 *   X . X           126]        (0b01111110)
 *   X . X
 *   X . X          This creates a nice rounded '0' shape
 *   X . X
 *   X . X
 *   . X .
 *
 * WHY WE CAN'T USE A 2D ARRAY:
 * ----------------------------
 * In C, arrays must have a fixed size for all dimensions except the first.
 * Since characters have VARIABLE widths (e.g., 'I' = 1 column, '0' = 3 columns),
 * we can't declare something like:
 *   uint8_t font[256][???][8];  // ??? = different width per character!
 *
 * Even if we could, it would waste tons of memory padding narrow characters
 * to match the widest one. Instead, we pack all characters tightly in a 1D
 * array and use the WIDTH and OFFSET tables to locate each character's data.
 */

/**
 * DEFAULT_FONT_FONT_DATA - Raw pixel data for all characters
 *
 * This is a packed array containing the column bytes for ALL characters.
 * Characters are stored back-to-back without separators.
 *
 * To find a character's data:
 *   1. Look up its offset in DEFAULT_FONT_CHAR_OFFSET[ascii_code]
 *   2. Look up its width in DEFAULT_FONT_CHAR_WIDTH[ascii_code]
 *   3. Read 'width' bytes starting at the offset
 *
 * Example: '0' (ASCII 48)
 *   - Offset: 144
 *   - Width: 3
 *   - Data: DEFAULT_FONT_FONT_DATA[144..146] = [126, 129, 126]
 *   - Visual: This draws a rounded rectangle (circle-ish shape for zero)
 */
static const uint8_t DEFAULT_FONT_FONT_DATA[] = {
    // ASCII 0-31: Control characters (empty)
    0,  // 0 (NULL)
    0,  // 1 (Start of Heading)
    0,  // 2 (Start of Text)
    0,  // 3 (End of Text)
    0,  // 4 (End of Transmission)
    0,  // 5 (Enquiry)
    0,  // 6 (Acknowledge)
    0,  // 7 (Bell)
    0,  // 8 (Backspace)
    0,  // 9 (Horizontal Tab)
    0,  // 10 (Line Feed)
    0,  // 11 (Vertical Tab)
    0,  // 12 (Form Feed)
    0,  // 13 (Carriage Return)
    0,  // 14 (Shift Out)
    0,  // 15 (Shift In)
    0,  // 16 (Data Link Escape)
    0,  // 17 (Device Control 1)
    0,  // 18 (Device Control 2)
    0,  // 19 (Device Control 3)
    0,  // 20 (Device Control 4)
    0,  // 21 (Negative Acknowledge)
    0,  // 22 (Synchronous Idle)
    0,  // 23 (End of Trans. Block)
    0,  // 24 (Cancel)
    0,  // 25 (End of Medium)
    0,  // 26 (Substitute)
    0,  // 27 (Escape)
    0,  // 28 (File Separator)
    0,  // 29 (Group Separator)
    0,  // 30 (Record Separator)
    0,  // 31 (Unit Separator)

    // ASCII 32-47: Space and symbols
    0,  // 32 ' ' (Space)
    94,  // 33 '!' (Exclamation mark)
    0,  // 34 '"' (Double quote)
    63, 192, 127, 192, 63, 0, 250, 0, 255, 9, 1, 0, 250,  // 35 '#' (Hash)
    72, 84, 36, 0, 12, 112, 12, 0, 124, 4, 120, 0, 56, 68, 68, 0,  // 36 '$' (Dollar sign)
    66, 37, 18, 72, 164, 66,  // 37 '%' (Percent)
    1,  // 38 '&' (Ampersand)
    6,  // 39 ''' (Single quote)
    254, 17, 17, 254, 0, 126, 129, 65, 190, 0, 129, 255, 129,  // 40 '(' (Left paren)
    130, 186, 198, 254, 134, 234, 134, 254, 250, 130, 250, 254, 134, 234, 134, 254, 124,  // 41 ')' (Right paren)
    250, 130, 250, 254, 130, 170, 186, 254, 130, 250, 226, 250, 134, 254, 130, 234, 234, 246, 254, 124,  // 42 '*' (Asterisk)
    0,  // 43 '+' (Plus)
    64, 0, 0,  // 44 ',' (Comma)
    8, 8,  // 45 '-' (Minus/Hyphen)
    128,  // 46 '.' (Period)
    130, 246, 238, 130, 254, 250, 130, 250, 254, 130, 234, 234, 246, 254, 124,  // 47 '/' (Slash)

    // ASCII 48-57: Digits 0-9
    126, 129, 126,  // 48 '0' - Rounded rectangle shape
    130, 255, 128,  // 49 '1' - Vertical line with flag
    194, 177, 142,  // 50 '2'
    66, 137, 118,  // 51 '3'
    15, 8, 255,  // 52 '4'
    79, 137, 113,  // 53 '5'
    126, 137, 114,  // 54 '6'
    1, 249, 7,  // 55 '7'
    118, 137, 118,  // 56 '8'
    78, 145, 126,  // 57 '9'

    // ASCII 58-64: More symbols
    36,  // 58 ':' (Colon)
    0,  // 59 ';' (Semicolon)
    0,  // 60 '<' (Less than)
    254, 17, 17, 254, 0, 255, 17, 17, 14,  // 61 '=' (Equals)
    0,  // 62 '>' (Greater than)
    124, 254, 254, 162, 254, 254, 254,  // 63 '?' (Question mark)
    250,  // 64 '@' (At sign)

    // ASCII 65-90: Uppercase letters A-Z
    124, 10, 124,  // 65 'A'
    126, 74, 52,  // 66 'B'
    60, 66, 66,  // 67 'C'
    126, 66, 60,  // 68 'D'
    126, 74, 66,  // 69 'E'
    126, 10, 2,  // 70 'F'
    60, 82, 116,  // 71 'G'
    126, 8, 126,  // 72 'H'
    126,  // 73 'I'
    32, 64, 62,  // 74 'J'
    126, 8, 118,  // 75 'K'
    126, 64, 64,  // 76 'L'
    126, 4, 126,  // 77 'M'
    126, 2, 124,  // 78 'N'
    60, 66, 60,  // 79 'O'
    126, 18, 12,  // 80 'P'
    60, 66, 124,  // 81 'Q'
    126, 18, 108,  // 82 'R'
    68, 74, 50,  // 83 'S'
    2, 126, 2,  // 84 'T'
    62, 64, 62,  // 85 'U'
    30, 96, 30,  // 86 'V'
    126, 32, 126,  // 87 'W'
    118, 8, 118,  // 88 'X'
    6, 120, 6,  // 89 'Y'
    98, 90, 70,  // 90 'Z'

    // ASCII 91-96: More symbols
    126, 129, 129, 66,  // 91 '[' (Left bracket)
    6, 28, 48,  // 92 '\' (Backslash)
    255, 9, 9, 1,  // 93 ']' (Right bracket)
    8,  // 94 '^' (Caret)
    32, 32, 32,  // 95 '_' (Underscore)
    255, 8, 20, 227,  // 96 '`' (Backtick)

    // ASCII 97-122: Lowercase letters a-z
    249, 21, 249,  // 97 'a'
    253, 149, 105,  // 98 'b'
    121, 133, 73,  // 99 'c'
    253, 133, 121,  // 100 'd'
    253, 149, 133,  // 101 'e'
    253, 21, 5,  // 102 'f'
    121, 165, 233,  // 103 'g'
    253, 17, 253,  // 104 'h'
    1, 253, 1,  // 105 'i'
    65, 129, 125,  // 106 'j'
    253, 17, 237,  // 107 'k'
    253, 129, 129,  // 108 'l'
    253, 9, 253,  // 109 'm'
    253, 5, 249,  // 110 'n'
    121, 133, 121,  // 111 'o'
    253, 37, 25,  // 112 'p'
    121, 133, 249,  // 113 'q'
    253, 37, 217,  // 114 'r'
    137, 149, 101,  // 115 's'
    5, 253, 5,  // 116 't'
    125, 129, 125,  // 117 'u'
    61, 193, 61,  // 118 'v'
    253, 65, 253,  // 119 'w'
    237, 17, 237,  // 120 'x'
    13, 241, 13,  // 121 'y'
    197, 181, 141,  // 122 'z'

    // ASCII 123-126: More symbols
    255, 253, 129, 253, 255, 129, 255, 129, 251, 129, 255, 129, 181, 189, 255, 249,  // 123 '{' (Left brace)
    255, 187, 181, 205, 255, 255, 193, 191, 193, 255, 129, 237, 243, 255, 161, 255,  // 124 '|' (Vertical bar)
    0, 2, 126, 2, 0, 126, 0, 126, 4, 126, 0, 126, 74, 66, 0, 6,  // 125 '}' (Right brace)
    0, 68, 74, 50, 0, 0, 62, 64, 62, 0, 126, 18, 12, 0, 94, 0,  // 126 '~' (Tilde)

    // ASCII 127-255: Extended characters (mostly empty or special symbols)
    13, 17, 253,  // 132
    253, 149, 101,  // 133
    4, 126, 4,  // 134
    4, 126, 4, 0, 4, 126, 4,  // 135
    32, 126, 32,  // 136
    32, 126, 32, 0, 32, 126, 32,  // 137
    0, 64, 32, 16, 10, 6, 14, 0,  // 138
    0, 8, 8, 8, 8, 28, 8, 0,  // 139
    0, 2, 4, 8, 80, 96, 112, 0,  // 140
    4, 126,  // 145
    100, 82, 76,  // 146
    66, 74, 52,  // 147
    14, 8, 126,  // 148
    78, 74, 50,  // 149
    60, 74, 52,  // 150
    2, 122, 6,  // 151
    52, 74, 52,  // 152
    12, 82, 60,  // 153
    60, 66, 60,  // 154
    227, 151, 143, 151, 227,  // 161
    227, 149, 157, 149, 227,  // 162
    227, 181, 185, 181, 227,  // 163
    227, 245, 249, 245, 227,  // 164
    224, 224, 0, 0, 0, 0, 0, 0,  // 169
    224, 224, 0, 252, 252, 0, 0, 0,  // 170
    224, 224, 0, 252, 252, 0, 255, 255,  // 171
    64, 0, 0, 0, 0,  // 174
    64, 0, 64, 0, 0,  // 175
    64, 0, 64, 0, 64,  // 176
    254, 146, 146, 146, 254,  // 177
    128, 126, 42, 42, 170, 254,  // 178
    128, 152, 64, 62, 80, 136, 128,  // 179
    72, 40, 152, 254, 16, 40, 68,  // 180
    68, 36, 20, 254, 20, 36, 68,  // 181
    168, 232, 172, 250, 172, 232, 168,  // 182
    128, 136, 136, 254, 136, 136, 128,  // 183
    4, 10, 4,  // 186
};

/**
 * DEFAULT_FONT_CHAR_WIDTH - Width of each character in columns
 *
 * For each ASCII code (0-255), this tells you how many columns wide the character is.
 * A width of 0 means the character is not defined (empty).
 *
 * Example usage:
 *   char ch = '0';
 *   uint8_t width = DEFAULT_FONT_CHAR_WIDTH[ch];  // Returns 3
 *   // This means the digit '0' is 3 columns wide
 *
 * Common widths:
 *   - Most digits (0-9): 3 columns
 *   - Most uppercase letters (A-Z): 3 columns
 *   - Most lowercase letters (a-z): 3 columns
 *   - 'I': 1 column (narrowest letter)
 *   - Some symbols: variable width (1-20 columns)
 */
static const uint8_t DEFAULT_FONT_CHAR_WIDTH[256] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 0-15
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 16-31
    1, 1, 1, 13, 16, 6, 1, 1, 13, 17, 20, 1, 3, 2, 1, 15,  // 32-47 (Space, !, ", #, $, %, etc.)
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 1, 9, 1, 7,  // 48-63 (0-9, :, ;, <, =, >, ?, @)
    1, 3, 3, 3, 3, 3, 3, 3, 3, 1, 3, 3, 3, 3, 3, 3,  // 64-79 (@, A-O)
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 3, 4, 1, 3,  // 80-95 (P-Z, [, \, ], ^, _, `)
    4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,  // 96-111 (`, a-o)
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 16, 16, 16, 16, 0,  // 112-127 (p-z, {, |, }, ~, DEL)
    0, 0, 0, 0, 3, 3, 3, 7, 3, 7, 8, 8, 8, 0, 0, 0,  // 128-143
    0, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0,  // 144-159
    0, 5, 5, 5, 5, 0, 0, 0, 0, 8, 8, 8, 0, 0, 5, 5,  // 160-175
    5, 5, 6, 7, 7, 7, 7, 7, 0, 0, 3, 0, 0, 0, 0, 0,  // 176-191
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 192-207
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 208-223
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 224-239
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 240-255
};

/**
 * DEFAULT_FONT_CHAR_OFFSET - Starting position of each character's data
 *
 * This tells you where in DEFAULT_FONT_FONT_DATA each character's column bytes begin.
 * Since characters have variable widths, they're packed tightly in DEFAULT_FONT_FONT_DATA,
 * and we need this offset table to find where each one starts.
 *
 * HOW IT WORKS:
 * -------------
 * Imagine DEFAULT_FONT_FONT_DATA as a long tape with all characters back-to-back:
 *
 *   [char0][char1][...][char48='0'][char49='1'][char50='2'][...]
 *    ^      ^           ^           ^           ^
 *    0      1           144         147         150
 *
 * The offset for '0' (ASCII 48) is 144, meaning:
 *   - The first byte of '0' is at DEFAULT_FONT_FONT_DATA[144]
 *   - Since '0' has width=3, it occupies bytes 144, 145, 146
 *   - The next character '1' (ASCII 49) starts at offset 147
 *
 * Example calculation for '1' (ASCII 49):
 *   offset = DEFAULT_FONT_CHAR_OFFSET[49] = 147
 *   width  = DEFAULT_FONT_CHAR_WIDTH[49]  = 3
 *   data   = &DEFAULT_FONT_FONT_DATA[147]  // Points to [130, 255, 128]
 *
 * WHY WE NEED OFFSETS:
 * --------------------
 * Without offsets, we'd have to sum up all previous character widths:
 *   offset_of_49 = width[0] + width[1] + ... + width[48]  // SLOW!
 *
 * With the offset table:
 *   offset_of_49 = DEFAULT_FONT_CHAR_OFFSET[49]  // Fast lookup!
 */
static const uint16_t DEFAULT_FONT_CHAR_OFFSET[256] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,  // 0-15
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,  // 16-31
    32, 33, 34, 35, 48, 64, 70, 71, 72, 85, 102, 122, 123, 126, 128, 129,  // 32-47
    144, 147, 150, 153, 156, 159, 162, 165, 168, 171, 174, 175, 176, 177, 186, 187,  // 48-63 ('0' starts at 144)
    194, 195, 198, 201, 204, 207, 210, 213, 216, 219, 220, 223, 226, 229, 232, 235,  // 64-79
    238, 241, 244, 247, 250, 253, 256, 259, 262, 265, 268, 271, 275, 278, 282, 283,  // 80-95
    286, 290, 293, 296, 299, 302, 305, 308, 311, 314, 317, 320, 323, 326, 329, 332,  // 96-111
    335, 338, 341, 344, 347, 350, 353, 356, 359, 362, 365, 368, 384, 400, 416, 432,  // 112-127
    432, 432, 432, 432, 432, 435, 438, 441, 448, 451, 458, 466, 474, 482, 482, 482,  // 128-143
    482, 482, 484, 487, 490, 493, 496, 499, 502, 505, 508, 511, 511, 511, 511, 511,  // 144-159
    511, 511, 516, 521, 526, 531, 531, 531, 531, 531, 539, 547, 555, 555, 555, 560,  // 160-175
    565, 570, 575, 581, 588, 595, 602, 609, 616, 616, 616, 619, 619, 619, 619, 619,  // 176-191
    619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619,  // 192-207
    619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619,  // 208-223
    619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619,  // 224-239
    619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619, 619,  // 240-255
};

/**
 * @brief Get character font data by ASCII code
 *
 * This function looks up a character and returns a pointer to its description,
 * which includes:
 *   - width: How many columns wide the character is
 *   - data: Pointer to the column bytes in DEFAULT_FONT_FONT_DATA
 *
 * @param ch ASCII code of the character (0-255)
 * @return Pointer to character descriptor with width and column data
 *
 * Example:
 *   const font_char_t *zero = default_font_get_char('0');
 *   // zero->width = 3
 *   // zero->data points to [126, 129, 126]
 */
const font_char_t* default_font_get_char(uint8_t ch)
{
    static font_char_t char_desc;
    static uint8_t char_data_with_spacing[21];  // Max width 20 + 1 spacing

    uint8_t base_width = DEFAULT_FONT_CHAR_WIDTH[ch];

    if (base_width == 0) {
        // Empty character
        char_desc.width = 0;
        char_desc.data = NULL;
        return &char_desc;
    }

    // Copy character data and add spacing column
    const uint8_t *base_data = &DEFAULT_FONT_FONT_DATA[DEFAULT_FONT_CHAR_OFFSET[ch]];
    for (int i = 0; i < base_width; i++) {
        char_data_with_spacing[i] = base_data[i];
    }
    char_data_with_spacing[base_width] = 0x00;  // Add empty spacing column

    // Return character with spacing included
    char_desc.width = base_width + 1;
    char_desc.data = char_data_with_spacing;

    return &char_desc;
}

// Font descriptor for the default font
const font_t font_default = {
    .name = "default",
    .get_char = default_font_get_char,
    .get_char_last = default_font_get_char  // Same as get_char (spacing is invisible anyway)
};
