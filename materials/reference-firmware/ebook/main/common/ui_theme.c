#include "ui_theme.h"
#include "app_ebook.h"

/* 逻辑坐标矩形 → 物理 framebuffer（90° 旋转） */
void ui_fill_logical_rect(uint8_t* fb, int lx, int ly, int lw, int lh, uint8_t color)
{
    if (lw <= 0 || lh <= 0) return;
    EpdRect r = {
        .x = ly,
        .y = LOGICAL_SCREEN_W - lx - lw,
        .width = lh,
        .height = lw,
    };
    epd_fill_rect(r, color, fb);
}

static void ui_draw_line_logical(uint8_t* fb, int lx1, int ly1, int lx2, int ly2, uint8_t color)
{
    int px1, py1, px2, py2;
    logical_to_physical(lx1, ly1, &px1, &py1);
    logical_to_physical(lx2, ly2, &px2, &py2);
    epd_draw_line(px1, py1, px2, py2, color, fb);
}

void ui_draw_chevron_right(uint8_t* fb, int cx, int cy, uint8_t color)
{
    const int s = 10;
    ui_draw_line_logical(fb, cx - s, cy - s, cx, cy, color);
    ui_draw_line_logical(fb, cx, cy, cx - s, cy + s, color);
}

void ui_draw_chevron_left(uint8_t* fb, int cx, int cy, uint8_t color)
{
    const int s = 10;
    ui_draw_line_logical(fb, cx + s, cy - s, cx, cy, color);
    ui_draw_line_logical(fb, cx, cy, cx + s, cy + s, color);
}

void ui_draw_text(uint8_t* fb, int x, int y, const char* text, uint8_t color)
{
    const char* p = text;
    int cx = x;
    while (*p) {
        uint16_t u = decode_utf8(&p);
        if (!u) break;
        ebook_draw_char(fb, cx, y, u, color);
        cx += (u < 128) ? 14 : 24;
    }
}

void ui_draw_text_truncated(uint8_t* fb, int x, int y, int max_w, const char* text, uint8_t color)
{
    const char* p = text;
    int cx = x;
    while (*p) {
        const char* next = p;
        uint16_t u = decode_utf8(&next);
        if (!u) break;
        int cw = (u < 128) ? 14 : 24;
        if (cx + cw > x + max_w) break;
        ebook_draw_char(fb, cx, y, u, color);
        cx += cw;
        p = next;
    }
}

void ui_draw_text_centered(uint8_t* fb, int y, const char* text, uint8_t color)
{
    int w = 0;
    const char* p = text;
    while (*p) {
        uint16_t u = decode_utf8(&p);
        if (!u) break;
        w += (u < 128) ? 14 : 24;
    }
    int x = (LOGICAL_SCREEN_W - w) / 2;
    if (x < UI_PAD_X) x = UI_PAD_X;
    ui_draw_text(fb, x, y, text, color);
}

void ui_draw_text_right(uint8_t* fb, int right_x, int y, const char* text, uint8_t color)
{
    int w = 0;
    const char* p = text;
    while (*p) {
        uint16_t u = decode_utf8(&p);
        if (!u) break;
        w += (u < 128) ? 14 : 24;
    }
    int x = right_x - w;
    if (x < UI_PAD_X) x = UI_PAD_X;
    ui_draw_text(fb, x, y, text, color);
}

void ui_draw_hline(uint8_t* fb, int y, uint8_t color)
{
    int lx1, ly1, lx2, ly2;
    logical_to_physical(UI_PAD_X, y, &lx1, &ly1);
    logical_to_physical(LOGICAL_SCREEN_W - UI_PAD_X, y, &lx2, &ly2);
    epd_draw_line(lx1, ly1, lx2, ly2, color, fb);
}

void ui_draw_header(uint8_t* fb, const char* title, const char* right, bool show_back)
{
    int title_x = UI_PAD_X;
    if (show_back) {
        ui_draw_chevron_left(fb, UI_PAD_X - 8, UI_STATUS_Y + 14, 0x00);
        title_x = UI_PAD_X + 16;
    }
    ui_draw_text(fb, title_x, UI_STATUS_Y, title, 0x00);
    if (right && right[0]) {
        ui_draw_text_right(fb, LOGICAL_SCREEN_W - UI_PAD_X, UI_STATUS_Y, right, 0x80);
    }
    ui_draw_hline(fb, UI_STATUS_Y + 36, 0xC0);
}

void ui_draw_status_bar(uint8_t* fb, const char* title, const char* right)
{
    ui_draw_header(fb, title, right, false);
}

void ui_draw_menu_row(uint8_t* fb, int y, const char* label, bool selected)
{
    const int row_h = UI_LIST_ROW_H - 4;
    const int text_max = LOGICAL_SCREEN_W - UI_PAD_X * 2 - 40;

    if (selected) {
        ui_fill_logical_rect(fb, 0, y - 10, LOGICAL_SCREEN_W, row_h, 0x00);
        ui_draw_text_truncated(fb, UI_PAD_X, y, text_max, label, 0xFF);
        ui_draw_chevron_right(fb, LOGICAL_SCREEN_W - UI_PAD_X, y + 14, 0xFF);
    } else {
        ui_fill_logical_rect(fb, 0, y - 10, LOGICAL_SCREEN_W, row_h, 0xFF);
        ui_draw_text_truncated(fb, UI_PAD_X, y, text_max, label, 0x00);
    }
}

void ui_draw_footer(uint8_t* fb, const char* hint)
{
    ui_fill_logical_rect(fb, 0, UI_FOOTER_Y - 32,
                         LOGICAL_SCREEN_W, LOGICAL_SCREEN_H - (UI_FOOTER_Y - 32), 0xFF);
    ui_draw_hline(fb, UI_FOOTER_Y - 20, 0xC0);
    ui_draw_text_centered(fb, UI_FOOTER_Y, hint, 0x00);
}

void ui_frame_begin(EpdiyHighlevelState* hl, bool full_clear)
{
    epd_poweron();
    if (full_clear) {
        /* 仅清内存 framebuffer，避免 epd_fullclear 先全刷白屏再刷内容 */
        epd_hl_set_all_white(hl);
    }
}

void ui_panel_clear(EpdiyHighlevelState* hl)
{
    epd_poweron();
    epd_fullclear(hl, 25);
    epd_poweroff();
}

void ui_present(EpdiyHighlevelState* hl, enum EpdDrawMode mode)
{
    epd_hl_update_screen(hl, mode, 25);
    epd_poweroff();
}

void ui_present_full(EpdiyHighlevelState* hl)
{
    epd_hl_update_screen_full(hl, MODE_GC16, 25);
    epd_poweroff();
}

void ui_present_from_white(EpdiyHighlevelState* hl)
{
    epd_hl_update_screen_from_white(hl, MODE_GC16, 25);
    epd_poweroff();
}
