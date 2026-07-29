#ifndef UI_THEME_H
#define UI_THEME_H

#include "epd_highlevel.h"
#include "app_ebook.h"
#include <stdbool.h>
#include <stdint.h>

/* 布局（逻辑坐标，竖屏 684×1216） */
#define UI_PAD_X           56
#define UI_STATUS_Y        38
#define UI_TITLE_Y         105
#define UI_LIST_START_Y    142
#define UI_LIST_ROW_H      86
#define UI_FOOTER_Y        (LOGICAL_SCREEN_H - UI_PAD_X)

/* 阅读器全刷间隔（局刷次数） */
#define UI_READER_GC16_INTERVAL  30

/* 菜单/列表局刷；阅读器翻页也使用 */
#define UI_PARTIAL_MODE          MODE_DU

void ui_fill_logical_rect(uint8_t* fb, int lx, int ly, int lw, int lh, uint8_t color);
void ui_draw_chevron_right(uint8_t* fb, int cx, int cy, uint8_t color);
void ui_draw_chevron_left(uint8_t* fb, int cx, int cy, uint8_t color);
void ui_draw_text(uint8_t* fb, int x, int y, const char* text, uint8_t color);
void ui_draw_text_truncated(uint8_t* fb, int x, int y, int max_w, const char* text, uint8_t color);
void ui_draw_text_centered(uint8_t* fb, int y, const char* text, uint8_t color);
void ui_draw_text_right(uint8_t* fb, int right_x, int y, const char* text, uint8_t color);
void ui_draw_hline(uint8_t* fb, int y, uint8_t color);
void ui_draw_header(uint8_t* fb, const char* title, const char* right, bool show_back);
void ui_draw_status_bar(uint8_t* fb, const char* title, const char* right);
void ui_draw_menu_row(uint8_t* fb, int y, const char* label, bool selected);
void ui_draw_footer(uint8_t* fb, const char* hint);
void ui_frame_begin(EpdiyHighlevelState* hl, bool full_clear);
void ui_panel_clear(EpdiyHighlevelState* hl);
void ui_present(EpdiyHighlevelState* hl, enum EpdDrawMode mode);
void ui_present_full(EpdiyHighlevelState* hl);
void ui_present_from_white(EpdiyHighlevelState* hl);

#endif /* UI_THEME_H */
