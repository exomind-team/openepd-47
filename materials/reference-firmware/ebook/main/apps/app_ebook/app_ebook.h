#ifndef APP_EBOOK_H
#define APP_EBOOK_H

#include "epdiy.h"
#include "epd_highlevel.h"

#define LOGICAL_SCREEN_W  684
#define LOGICAL_SCREEN_H  1216

static inline void logical_to_physical(int xL, int yL, int* xP, int* yP)
{
    *xP = yL;
    *yP = LOGICAL_SCREEN_W - 1 - xL;
}

void ebook_draw_char(uint8_t* fb, int x, int y, uint16_t unicode, uint8_t color);
uint16_t decode_utf8(const char** str);

#endif /* APP_EBOOK_H */
