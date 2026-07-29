#include "ui_manager.h"
#include "key_manager.h"
#include "factor_test.h"
#include "app_ebook.h"
#include "app_album.h"
#include "event_bus.h"
#include "wifi_manager.h"
#include "ebook_engine.h"
#include "book_storage.h"
#include "image_storage.h"
#include "ui_theme.h"
#include "wifi_config.h"
#include "epdiy.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include <string.h>
#include <stdio.h>

#define TAG "UI_MGR"

/* true = MODE_GC16 画面切换/内容大变；false = MODE_DU 同屏光标高亮 */
#define UI_DRAW_GC       true
#define UI_DRAW_PARTIAL  false

static bool demo_wifi_configured(void)
{
    return DEMO_WIFI_SSID[0] != '\0'
        && strcmp(DEMO_WIFI_SSID, "your_wifi_ssid") != 0;
}

// ---- 全局状态 ----
UIState_t g_ui_state = UI_STATE_HOME;

// ---- HOME 状态数据 ----
static int  s_home_cursor  = 1;   // 1=电子书, 2=电子相册

// ---- FILE_LIST 状态数据 ----
#define MAX_FILES 16
static char s_files[MAX_FILES][64];
static int  s_file_count   = 0;
static int  s_file_cursor  = 0;

// ---- READER 状态数据 ----
static char s_reader_path[128];
static char s_reader_open_path[128];
static bool s_reader_open_pending = false;
static int  s_reader_page = 0;
static int  s_reader_partial_count = 0;
static EbookEngine s_engine;

// ---- ALBUM 状态数据 ----
static char s_images[IMAGE_MAX_SLOTS][64];
static int  s_image_count   = 0;
static int  s_image_cursor  = 0;
static char s_viewer_path[128];

static int menu_row_y(int idx)
{
    return UI_LIST_START_Y + idx * UI_LIST_ROW_H;
}

// 首次启动强制重绘标志
static bool s_first_draw   = true;

// ============================================================
// 按键轮询
// ============================================================
static UIKey_t poll_key(void)
{
    if (g_key_up_pressed)    { g_key_up_pressed    = false; return UI_KEY_UP;      }
    if (g_key_down_pressed)  { g_key_down_pressed  = false; return UI_KEY_DOWN;    }
    if (g_key_set_pressed)   { g_key_set_pressed   = false; return UI_KEY_CONFIRM; }
    if (g_key_fresh_pressed) { g_key_fresh_pressed = false; return UI_KEY_CANCEL;  }
    return UI_KEY_NONE;
}

static void drain_keys(void)
{
    while (poll_key() != UI_KEY_NONE) {
    }
}

// ============================================================
// HOME — 主菜单绘制
// ============================================================
static void draw_home(EpdiyHighlevelState* hl, bool full, bool from_white)
{
    ui_frame_begin(hl, full || from_white);
    uint8_t* fb = epd_hl_get_framebuffer(hl);
    if (!full && !from_white) {
        epd_fill_rect(epd_full_screen(), 0xFF, fb);
    }

    const char* wifi = (g_wifi_state == 2) ? g_wifi_ip : "";
    ui_draw_header(fb, "图书馆", wifi, false);

    const char* apps[] = {"我的书架", "我的相册"};
    for (int i = 0; i < 2; i++) {
        ui_draw_menu_row(fb, menu_row_y(i), apps[i], s_home_cursor == i + 1);
    }

    if (g_wifi_state == 2) {
        char url[72];
        snprintf(url, sizeof(url), "传书：http://%s/", g_wifi_ip);
        ui_draw_footer(fb, url);
    } else if (g_wifi_state == 1) {
        ui_draw_footer(fb, "Wi-Fi 连接中...");
    } else if (demo_wifi_configured()) {
        ui_draw_footer(fb, "Wi-Fi 连接失败，请检查热点");
    } else {
        ui_draw_footer(fb, "请配置 wifi_config.h 后重新烧录");
    }

    if (from_white) {
        ui_present_from_white(hl);
    } else if (full) {
        ui_present_full(hl);
    } else {
        ui_present(hl, UI_PARTIAL_MODE);
    }
}

// ============================================================
// FILE_LIST — 扫描 /spiffs/books/*.txt
// ============================================================
static void scan_files(void)
{
    s_file_count  = 0;
    s_file_cursor = 0;
    s_file_count = book_storage_scan(s_files, MAX_FILES);
}

static void format_book_row(char* row, size_t row_size, const char* path)
{
    const char* name = path;
    const char* slash = strrchr(path, '/');
    if (slash) name = slash + 1;

    strncpy(row, name, row_size - 1);
    row[row_size - 1] = '\0';
    size_t rlen = strlen(row);
    if (rlen > 4 && strcasecmp(row + rlen - 4, ".txt") == 0) {
        row[rlen - 4] = '\0';
    }
}

static void draw_file_list(EpdiyHighlevelState* hl, bool full)
{
    ui_frame_begin(hl, full);
    uint8_t* fb = epd_hl_get_framebuffer(hl);
    if (!full) {
        epd_fill_rect(epd_full_screen(), 0xFF, fb);
    }

    char count_str[16];
    snprintf(count_str, sizeof(count_str), "%d 本", s_file_count);
    ui_draw_header(fb, "我的书架", count_str, true);

    if (s_file_count == 0) {
        ui_draw_text_centered(fb, 570,
            "暂无书籍，请通过浏览器传书", 0x00);
        if (g_wifi_state == 2) {
            char hint[80];
            snprintf(hint, sizeof(hint), "http://%s/", g_wifi_ip);
            ui_draw_text_centered(fb, 665, hint, 0x80);
        }
    } else {
        for (int i = 0; i < s_file_count; i++) {
            int y = menu_row_y(i);
            if (y > 1045) break;

            char row[56];
            format_book_row(row, sizeof(row), s_files[i]);

            ui_draw_menu_row(fb, y, row, i == s_file_cursor);
        }
    }

    char footer[80];
    book_storage_format_usage(footer, sizeof(footer));
    ui_draw_footer(fb, footer);

    if (full) {
        ui_present_full(hl);
    } else {
        ui_present(hl, UI_PARTIAL_MODE);
    }
}

// ============================================================
// ALBUM — 图片列表 / 查看 / 删除
// ============================================================
static void scan_images(void)
{
    s_image_count  = 0;
    s_image_cursor = 0;
    s_image_count = image_storage_scan(s_images, IMAGE_MAX_SLOTS);
}

static void format_image_row(char* row, size_t row_size, const char* path)
{
    const char* name = path;
    const char* slash = strrchr(path, '/');
    if (slash) {
        name = slash + 1;
    }

    strncpy(row, name, row_size - 1);
    row[row_size - 1] = '\0';
    size_t rlen = strlen(row);
    if (rlen > 4 && strcasecmp(row + rlen - 4, ".raw") == 0) {
        row[rlen - 4] = '\0';
    }
}

static void draw_album_list(EpdiyHighlevelState* hl, bool full)
{
    ui_frame_begin(hl, full);
    uint8_t* fb = epd_hl_get_framebuffer(hl);
    if (!full) {
        epd_fill_rect(epd_full_screen(), 0xFF, fb);
    }

    char count_str[16];
    snprintf(count_str, sizeof(count_str), "%d/%d", s_image_count, IMAGE_MAX_SLOTS);
    ui_draw_header(fb, "我的相册", count_str, true);

    if (s_image_count == 0) {
        ui_draw_text_centered(fb, 570, "暂无照片，请通过浏览器上传", 0x00);
        if (g_wifi_state == 2) {
            char hint[80];
            snprintf(hint, sizeof(hint), "http://%s/", g_wifi_ip);
            ui_draw_text_centered(fb, 665, hint, 0x80);
        }
    } else {
        for (int i = 0; i < s_image_count; i++) {
            int y = menu_row_y(i);
            if (y > 1045) {
                break;
            }

            char row[56];
            format_image_row(row, sizeof(row), s_images[i]);
            ui_draw_menu_row(fb, y, row, i == s_image_cursor);
        }
    }

    char footer[96];
    image_storage_format_usage(footer, sizeof(footer));
    ui_draw_footer(fb, footer);

    if (full) {
        ui_present_full(hl);
    } else {
        ui_present(hl, UI_PARTIAL_MODE);
    }
}

static void draw_album_view(EpdiyHighlevelState* hl, bool full)
{
    ui_frame_begin(hl, full);
    uint8_t* fb = epd_hl_get_framebuffer(hl);
    if (!full) {
        epd_fill_rect(epd_full_screen(), 0xFF, fb);
    }

    if (album_draw_file(hl, s_viewer_path)) {
        ui_draw_footer(fb, "取消=返回  确认=删除");
    } else {
        ui_draw_header(fb, "我的相册", "", true);
        ui_draw_text_centered(fb, 570, "无法打开图片", 0x00);
        ui_draw_footer(fb, "取消=返回");
    }

    ui_present(hl, full ? MODE_GC16 : UI_PARTIAL_MODE);
}

static void draw_album_delete_confirm(EpdiyHighlevelState* hl, bool full)
{
    ui_frame_begin(hl, full);
    uint8_t* fb = epd_hl_get_framebuffer(hl);
    if (!full) {
        epd_fill_rect(epd_full_screen(), 0xFF, fb);
    }

    char name[56];
    format_image_row(name, sizeof(name), s_viewer_path);

    char msg[72];
    snprintf(msg, sizeof(msg), "删除 %s ?", name);
    ui_draw_text_centered(fb, 570, msg, 0x00);
    ui_draw_footer(fb, "确认=删除  取消=返回");

    ui_present(hl, full ? MODE_GC16 : UI_PARTIAL_MODE);
}

// ============================================================
// READER — 打开中提示
// ============================================================
static void draw_loading_box(uint8_t* fb, const char* line1, const char* line2)
{
    const int box_x = 80;
    const int box_y = 451;
    const int box_w = LOGICAL_SCREEN_W - 160;
    const int box_h = 261;

    ui_fill_logical_rect(fb, box_x, box_y, box_w, box_h, 0x00);
    ui_fill_logical_rect(fb, box_x + 2, box_y + 2, box_w - 4, box_h - 4, 0xFF);
    ui_draw_text_centered(fb, box_y + 72, line1, 0x00);
    if (line2 && line2[0]) {
        ui_draw_text_centered(fb, box_y + 136, line2, 0x00);
    }
}

static void draw_reader_opening(EpdiyHighlevelState* hl, const char* path)
{
    epd_poweron();
    uint8_t* fb = epd_hl_get_framebuffer(hl);
    epd_fill_rect(epd_full_screen(), 0xFF, fb);

    char name[56];
    format_book_row(name, sizeof(name), path);
    draw_loading_box(fb, "正在打开书籍...", name);
    ui_draw_footer(fb, "请稍候，请勿按键");

    ui_present(hl, MODE_GC16);
}

// ============================================================
// READER — UTF-8 分页阅读器
// ============================================================
static void draw_reader(EpdiyHighlevelState* hl, bool full, enum EpdDrawMode mode)
{
    if (full) {
        epd_poweron();
        epd_hl_set_all_white(hl);
    } else {
        epd_poweron();
    }

    bool changed = true;
    if (!ebook_engine_render_page(&s_engine, hl, s_reader_page, s_reader_path, !full, &changed)) {
        uint8_t* fb = epd_hl_get_framebuffer(hl);
        if (!full) {
            epd_fill_rect(epd_full_screen(), 0xFF, fb);
        }
        ui_draw_text_centered(fb, 570, "无法打开文件", 0x00);
        changed = true;
    }

    if (!changed) {
        epd_poweroff();
        return;
    }

    ui_present(hl, mode);
}

static void reader_save_progress(void)
{
    uint32_t offset = s_engine.page_offsets[s_reader_page];
    ebook_engine_save_page(s_reader_path, s_reader_page, offset);
}

static void complete_reader_open(EpdiyHighlevelState* hl, const char* path)
{
    drain_keys();

    strncpy(s_reader_path, path, sizeof(s_reader_path) - 1);
    s_reader_path[sizeof(s_reader_path) - 1] = '\0';

    ebook_engine_init(&s_engine);
    if (!ebook_engine_open(&s_engine, s_reader_path)) {
        ESP_LOGW(TAG, "Failed to open book");
        g_ui_state = UI_STATE_FILE_LIST;
        draw_file_list(hl, UI_DRAW_GC);
        drain_keys();
        return;
    }

    uint32_t saved_offset = 0;
    s_reader_page = ebook_engine_load_saved_page(s_reader_path, &saved_offset);
    if (s_reader_page > 0 && saved_offset > 0) {
        ebook_engine_restore_position(&s_engine, s_reader_page, saved_offset);
    }

    g_ui_state = UI_STATE_READER;
    ESP_LOGI(TAG, "FILE_LIST -> READER: %s page %d", s_reader_path, s_reader_page);
    s_reader_partial_count = 0;
    draw_reader(hl, true, MODE_GC16);
    reader_save_progress();
    drain_keys();
}

static void begin_reader_open(EpdiyHighlevelState* hl, const char* path)
{
    strncpy(s_reader_open_path, path, sizeof(s_reader_open_path) - 1);
    s_reader_open_path[sizeof(s_reader_open_path) - 1] = '\0';
    s_reader_open_pending = true;
    g_ui_state = UI_STATE_READER_OPENING;
    draw_reader_opening(hl, s_reader_open_path);
    drain_keys();
}

// ============================================================
// 状态处理器
// ============================================================
static void on_home(EpdiyHighlevelState* hl, UIKey_t key)
{
    bool redraw = false;

    switch (key) {
    case UI_KEY_UP:
        if (--s_home_cursor < 1) s_home_cursor = 2;
        redraw = true;
        break;
    case UI_KEY_DOWN:
        if (++s_home_cursor > 2) s_home_cursor = 1;
        redraw = true;
        break;
    case UI_KEY_CONFIRM:
        if (s_home_cursor == 1) {
            ESP_LOGI(TAG, "HOME -> FILE_LIST");
            scan_files();
            g_ui_state = UI_STATE_FILE_LIST;
            draw_file_list(hl, UI_DRAW_GC);
        } else {
            ESP_LOGI(TAG, "HOME -> ALBUM_LIST");
            scan_images();
            g_ui_state = UI_STATE_ALBUM_LIST;
            draw_album_list(hl, UI_DRAW_GC);
        }
        return;
    default:
        break;
    }

    if (redraw) {
        draw_home(hl, false, false);
    }
}

static void on_file_list(EpdiyHighlevelState* hl, UIKey_t key)
{
    bool redraw = false;

    switch (key) {
    case UI_KEY_UP:
        if (s_file_cursor > 0) { s_file_cursor--; redraw = true; }
        break;
    case UI_KEY_DOWN:
        if (s_file_cursor < s_file_count - 1) { s_file_cursor++; redraw = true; }
        break;
    case UI_KEY_CONFIRM:
        if (s_file_count > 0) {
            begin_reader_open(hl, s_files[s_file_cursor]);
            return;
        }
        break;
    case UI_KEY_CANCEL:
        ESP_LOGI(TAG, "FILE_LIST -> HOME");
        g_ui_state = UI_STATE_HOME;
        draw_home(hl, true, false);
        return;
    default:
        break;
    }

    if (redraw) {
        draw_file_list(hl, false);
    }
}

static void on_reader(EpdiyHighlevelState* hl, UIKey_t key)
{
    bool redraw = false;

    switch (key) {
    case UI_KEY_UP:
        if (s_reader_page > 0) {
            s_reader_page--;
            redraw = true;
        }
        break;
    case UI_KEY_DOWN:
        if (ebook_engine_can_next(&s_engine, s_reader_page)) {
            s_reader_page++;
            redraw = true;
        }
        break;
    case UI_KEY_CANCEL:
        reader_save_progress();
        ebook_engine_close(&s_engine);
        for (int i = 0; i < s_file_count; i++) {
            if (strcmp(s_files[i], s_reader_path) == 0) {
                s_file_cursor = i;
                break;
            }
        }
        ESP_LOGI(TAG, "READER -> FILE_LIST");
        g_ui_state = UI_STATE_FILE_LIST;
        draw_file_list(hl, UI_DRAW_GC);
        return;
    default:
        break;
    }

    if (redraw) {
        enum EpdDrawMode mode;
        s_reader_partial_count++;
        bool gc = (s_reader_partial_count >= UI_READER_GC16_INTERVAL);
        if (gc) {
            s_reader_partial_count = 0;
            mode = MODE_GC16;
        } else {
            mode = UI_PARTIAL_MODE;
        }
        draw_reader(hl, gc, mode);
        reader_save_progress();
    }
}

static void on_album_list(EpdiyHighlevelState* hl, UIKey_t key)
{
    bool redraw = false;

    switch (key) {
    case UI_KEY_UP:
        if (s_image_cursor > 0) {
            s_image_cursor--;
            redraw = true;
        }
        break;
    case UI_KEY_DOWN:
        if (s_image_cursor < s_image_count - 1) {
            s_image_cursor++;
            redraw = true;
        }
        break;
    case UI_KEY_CONFIRM:
        if (s_image_count > 0) {
            strncpy(s_viewer_path, s_images[s_image_cursor], sizeof(s_viewer_path) - 1);
            s_viewer_path[sizeof(s_viewer_path) - 1] = '\0';
            g_ui_state = UI_STATE_ALBUM_VIEW;
            draw_album_view(hl, UI_DRAW_GC);
            return;
        }
        break;
    case UI_KEY_CANCEL:
        ESP_LOGI(TAG, "ALBUM_LIST -> HOME");
        g_ui_state = UI_STATE_HOME;
        draw_home(hl, true, false);
        return;
    default:
        break;
    }

    if (redraw) {
        draw_album_list(hl, false);
    }
}

static void on_album_view(EpdiyHighlevelState* hl, UIKey_t key)
{
    switch (key) {
    case UI_KEY_CONFIRM:
        g_ui_state = UI_STATE_ALBUM_DELETE;
        draw_album_delete_confirm(hl, UI_DRAW_GC);
        return;
    case UI_KEY_CANCEL:
        ESP_LOGI(TAG, "ALBUM_VIEW -> ALBUM_LIST");
        g_ui_state = UI_STATE_ALBUM_LIST;
        draw_album_list(hl, UI_DRAW_GC);
        return;
    default:
        break;
    }
}

static void on_album_delete(EpdiyHighlevelState* hl, UIKey_t key)
{
    switch (key) {
    case UI_KEY_CONFIRM:
        image_storage_delete(s_viewer_path);
        scan_images();
        if (s_image_cursor >= s_image_count && s_image_count > 0) {
            s_image_cursor = s_image_count - 1;
        }
        ESP_LOGI(TAG, "ALBUM_DELETE -> ALBUM_LIST");
        g_ui_state = UI_STATE_ALBUM_LIST;
        draw_album_list(hl, UI_DRAW_GC);
        return;
    case UI_KEY_CANCEL:
        g_ui_state = UI_STATE_ALBUM_VIEW;
        draw_album_view(hl, UI_DRAW_GC);
        return;
    default:
        break;
    }
}

static void handle_events(EpdiyHighlevelState* hl)
{
    AppEvent_t ev = event_poll();
    if (ev == EVT_NONE) return;

    switch (ev) {
    case EVT_WIFI_CHANGED:
        if (g_ui_state == UI_STATE_HOME) {
            draw_home(hl, false, false);
        } else if (g_ui_state == UI_STATE_ALBUM_LIST) {
            draw_album_list(hl, UI_DRAW_GC);
        } else if (g_ui_state == UI_STATE_FILE_LIST) {
            draw_file_list(hl, UI_DRAW_GC);
        }
        break;
    case EVT_BOOK_UPLOADED:
        if (g_ui_state == UI_STATE_FILE_LIST) {
            scan_files();
            draw_file_list(hl, UI_DRAW_GC);
        }
        break;
    case EVT_ALBUM_UPLOADED:
    case EVT_IMAGE_CHANGED:
        if (g_ui_state == UI_STATE_ALBUM_LIST) {
            scan_images();
            draw_album_list(hl, UI_DRAW_GC);
        } else if (g_ui_state == UI_STATE_ALBUM_VIEW) {
            scan_images();
            bool still_exists = false;
            for (int i = 0; i < s_image_count; i++) {
                if (strcmp(s_images[i], s_viewer_path) == 0) {
                    still_exists = true;
                    break;
                }
            }
            if (!still_exists) {
                if (s_image_cursor >= s_image_count && s_image_count > 0) {
                    s_image_cursor = s_image_count - 1;
                }
                g_ui_state = UI_STATE_ALBUM_LIST;
                draw_album_list(hl, UI_DRAW_GC);
            } else {
                draw_album_view(hl, UI_DRAW_GC);
            }
        }
        break;
    default:
        break;
    }
}

// ============================================================
// 公开 API
// ============================================================
void ui_manager_init(void)
{
    g_ui_state   = UI_STATE_HOME;
    s_home_cursor = 1;
    s_first_draw  = true;
    ESP_LOGI(TAG, "UI state machine initialized (HOME)");
}

bool ui_manager_check_factory_boot(EpdiyHighlevelState* hl)
{
    const int window_ms = 5000;
    const int need_hold_ms = 3000;
    int elapsed = 0;
    int held = 0;

    ESP_LOGI(TAG, "Hold FRESH 3s within 5s after boot for factory test");

    while (elapsed < window_ms) {
        if (key_manager_fresh_held()) {
            held += 100;
            if (held >= need_hold_ms) {
                ESP_LOGI(TAG, "Factory test triggered");
                factory_test_run(hl, 25);
                return true;
            }
        } else {
            held = 0;
        }
        elapsed += 100;
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    return false;
}

void ui_manager_tick(EpdiyHighlevelState* hl)
{
    // 首次启动：开机 ui_panel_clear 后单次 GC 显示主菜单
    if (s_first_draw) {
        s_first_draw = false;
        draw_home(hl, true, true);
    }

    if (g_ui_state == UI_STATE_READER_OPENING) {
        if (s_reader_open_pending) {
            s_reader_open_pending = false;
            complete_reader_open(hl, s_reader_open_path);
        } else {
            drain_keys();
        }
        return;
    }

    handle_events(hl);

    UIKey_t key = poll_key();
    if (key == UI_KEY_NONE) return;

    switch (g_ui_state) {
    case UI_STATE_HOME:           on_home(hl, key);           break;
    case UI_STATE_FILE_LIST:      on_file_list(hl, key);      break;
    case UI_STATE_READER:         on_reader(hl, key);         break;
    case UI_STATE_ALBUM_LIST:     on_album_list(hl, key);     break;
    case UI_STATE_ALBUM_VIEW:     on_album_view(hl, key);     break;
    case UI_STATE_ALBUM_DELETE:   on_album_delete(hl, key);   break;
    default:
        break;
    }
}