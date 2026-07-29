#include "app_ebook.h"
#include "my_chinese_font.h"
#include <string.h>

#define CHAR_SIZE 24

static const CustomGlyph* get_chinese_glyph(uint16_t unicode)
{
    int left = 0;
    int right = NUM_CHARS - 1;
    while (left <= right) {
        int mid = left + (right - left) / 2;
        if (my_font[mid].unicode == unicode) {
            return &my_font[mid];
        } else if (my_font[mid].unicode < unicode) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    return NULL;
}

void ebook_draw_char(uint8_t* fb, int x, int y, uint16_t unicode, uint8_t color)
{
    const CustomGlyph* glyph = get_chinese_glyph(unicode);
    if (!glyph) {
        epd_draw_rect((EpdRect){y, LOGICAL_SCREEN_W - 1 - x - CHAR_SIZE, CHAR_SIZE, CHAR_SIZE}, color, fb);
        return;
    }

    for (int row = 0; row < CHAR_SIZE; row++) {
        for (int col = 0; col < CHAR_SIZE; col++) {
            int byte_idx = row * 3 + (col / 8);
            int bit_idx = 7 - (col % 8);
            if ((glyph->bitmap[byte_idx] >> bit_idx) & 1) {
                int px, py;
                logical_to_physical(x + col, y + row, &px, &py);
                epd_draw_pixel(px, py, color, fb);
            }
        }
    }
}

uint16_t decode_utf8(const char** str)
{
    uint8_t c = (uint8_t)**str;
    if (c == 0) return 0;

    (*str)++;
    if (c < 0x80) return c;

    if ((c & 0xE0) == 0xC0) {
        uint8_t c2 = (uint8_t)**str;
        if (c2 == 0) return c;
        (*str)++;
        return ((c & 0x1F) << 6) | (c2 & 0x3F);
    }

    if ((c & 0xF0) == 0xE0) {
        uint8_t c2 = (uint8_t)**str;
        if (c2 == 0) return c;
        (*str)++;
        uint8_t c3 = (uint8_t)**str;
        if (c3 == 0) return c;
        (*str)++;
        return ((c & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
    }

    return '?';
}
