#include "ebook_engine.h"
#include "app_ebook.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "nvs.h"
#include <stdio.h>
#include <string.h>

#define TAG "EBOOK_ENG"

#define CHAR_SIZE 24
#define LINE_SPACING 14
#define READER_MARGIN_X  56
#define READER_MARGIN_Y  105
#define READER_BOTTOM_PAD 114
#define PARA_INDENT      (2 * CHAR_SIZE)

static bool is_paragraph_start_at_offset(const char* path, uint32_t offset)
{
    if (offset == 0) return true;

    FILE* fd = fopen(path, "rb");
    if (!fd) return false;

    uint32_t scan_len = offset < 128 ? offset : 128;
    uint32_t start = offset - scan_len;
    fseek(fd, start, SEEK_SET);
    uint8_t buf[128];
    size_t n = fread(buf, 1, scan_len, fd);
    fclose(fd);

    if (n == 0) return true;

    for (int i = (int)n - 1; i >= 0; i--) {
        uint8_t c = buf[i];
        if (c == '\n' || c == '\r') {
            return true;
        }
        if (c == ' ' || c == '\t') {
            continue;
        }
        if (c == 0x80 && i >= 2 && buf[i - 1] == 0x80 && buf[i - 2] == 0xE3) {
            i -= 2;
            continue;
        }
        return false;
    }
    return start == 0;
}

static void layout_begin_line(int margin_x, bool* para_start, int* cursor_x)
{
    if (*para_start) {
        *cursor_x = margin_x + PARA_INDENT;
        *para_start = false;
    }
}

static void layout_wrap_line(int* cursor_x, int* cursor_y, int margin_x)
{
    *cursor_x = margin_x;
    *cursor_y += CHAR_SIZE + LINE_SPACING;
}

static uint32_t djb2_hash(const char* str)
{
    uint32_t hash = 5381;
    while (*str) {
        hash = ((hash << 5) + hash) + (uint8_t)*str++;
    }
    return hash;
}

static uint16_t fdecode_utf8(FILE* fd)
{
    int c = fgetc(fd);
    if (c == EOF) return 0;
    uint8_t u8 = (uint8_t)c;

    if (u8 == 0) return '?';
    if (u8 < 0x80) return u8;

    if ((u8 & 0xE0) == 0xC0) {
        int c2 = fgetc(fd);
        if (c2 == EOF) return 0;
        if ((c2 & 0xC0) != 0x80) {
            fseek(fd, -1, SEEK_CUR);
            return '?';
        }
        return ((u8 & 0x1F) << 6) | (c2 & 0x3F);
    }

    if ((u8 & 0xF0) == 0xE0) {
        int c2 = fgetc(fd);
        if (c2 == EOF) return 0;
        if ((c2 & 0xC0) != 0x80) {
            fseek(fd, -1, SEEK_CUR);
            return '?';
        }
        int c3 = fgetc(fd);
        if (c3 == EOF) return 0;
        if ((c3 & 0xC0) != 0x80) {
            fseek(fd, -1, SEEK_CUR);
            return '?';
        }
        return ((u8 & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
    }

    return '?';
}

static void draw_text_line(uint8_t* fb, int x, int y, const char* text, uint8_t color)
{
    const char* p = text;
    int cx = x;
    while (*p) {
        uint16_t u = decode_utf8(&p);
        if (!u) break;
        ebook_draw_char(fb, cx, y, u, color);
        cx += (u < 128) ? 14 : CHAR_SIZE;
    }
}

static uint32_t paginate_page_at_offset(const char* path, uint32_t start_offset)
{
    int screen_w = LOGICAL_SCREEN_W;
    int screen_h = LOGICAL_SCREEN_H;
    int margin_x = READER_MARGIN_X;
    int margin_y = READER_MARGIN_Y;

    FILE* fd = fopen(path, "r");
    if (!fd) return start_offset;

    fseek(fd, start_offset, SEEK_SET);

    int cursor_x = margin_x;
    int cursor_y = margin_y;
    uint32_t current_pos = start_offset;
    bool para_start = is_paragraph_start_at_offset(path, start_offset);

    while (1) {
        long prev_pos = ftell(fd);
        uint16_t u = fdecode_utf8(fd);
        if (u == 0) break;

        uint32_t byte_len = (uint32_t)(ftell(fd) - prev_pos);
        current_pos += byte_len;

        if (u == '\n') {
            cursor_x = margin_x;
            cursor_y += CHAR_SIZE + LINE_SPACING;
            para_start = true;
            continue;
        }

        int char_w = (u < 128) ? 14 : CHAR_SIZE;

        layout_begin_line(margin_x, &para_start, &cursor_x);

        if (cursor_x + char_w > screen_w - margin_x) {
            layout_wrap_line(&cursor_x, &cursor_y, margin_x);
        }

        if (cursor_y + CHAR_SIZE > screen_h - READER_BOTTOM_PAD) {
            fseek(fd, -(long)byte_len, SEEK_CUR);
            current_pos -= byte_len;
            break;
        }

        if (u != '\r') {
            cursor_x += char_w;
        }
    }

    fclose(fd);
    return current_pos;
}

static uint8_t* s_render_scratch = NULL;
static size_t s_render_scratch_bytes = 0;

static uint8_t* get_render_scratch(size_t bytes)
{
    if (s_render_scratch && s_render_scratch_bytes >= bytes) {
        return s_render_scratch;
    }
    if (s_render_scratch) {
        free(s_render_scratch);
        s_render_scratch = NULL;
        s_render_scratch_bytes = 0;
    }
    s_render_scratch = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!s_render_scratch) {
        s_render_scratch = malloc(bytes);
    }
    if (!s_render_scratch) {
        return NULL;
    }
    s_render_scratch_bytes = bytes;
    return s_render_scratch;
}

static bool merge_framebuffer_diff(uint8_t* front, const uint8_t* desired, size_t byte_len)
{
    bool any = false;
    for (size_t i = 0; i < byte_len; i++) {
        uint8_t f = front[i];
        uint8_t d = desired[i];
        if (f == d) {
            continue;
        }

        uint8_t out = f;
        if ((f >> 4) != (d >> 4)) {
            out = (uint8_t)((out & 0x0F) | (d & 0xF0));
        }
        if ((f & 0x0F) != (d & 0x0F)) {
            out = (uint8_t)((out & 0xF0) | (d & 0x0F));
        }
        front[i] = out;
        any = true;
    }
    return any;
}

static void render_page_at_offset(uint8_t* fb, const char* path, uint32_t start_offset,
                                  uint32_t end_pos, const char* header, int page_num)
{
    int screen_w = LOGICAL_SCREEN_W;
    int screen_h = LOGICAL_SCREEN_H;
    int margin_x = READER_MARGIN_X;
    int margin_y = READER_MARGIN_Y;

    epd_fill_rect(epd_full_screen(), 0xFF, fb);

    draw_text_line(fb, READER_MARGIN_X, 36, header, 0x80);

    int lx1, ly1, lx2, ly2;
    logical_to_physical(READER_MARGIN_X, 72, &lx1, &ly1);
    logical_to_physical(screen_w - READER_MARGIN_X, 72, &lx2, &ly2);
    epd_draw_line(lx1, ly1, lx2, ly2, 0xC0, fb);

    FILE* fd = fopen(path, "r");
    if (!fd) return;

    fseek(fd, start_offset, SEEK_SET);

    int cursor_x = margin_x;
    int cursor_y = margin_y;
    uint32_t pos = start_offset;
    bool para_start = is_paragraph_start_at_offset(path, start_offset);

    while (pos < end_pos) {
        long prev_pos = ftell(fd);
        uint16_t u = fdecode_utf8(fd);
        if (u == 0) break;

        uint32_t byte_len = (uint32_t)(ftell(fd) - prev_pos);
        pos += byte_len;

        if (u == '\n') {
            cursor_x = margin_x;
            cursor_y += CHAR_SIZE + LINE_SPACING;
            para_start = true;
            continue;
        }

        int char_w = (u < 128) ? 14 : CHAR_SIZE;

        layout_begin_line(margin_x, &para_start, &cursor_x);

        if (cursor_x + char_w > screen_w - margin_x) {
            layout_wrap_line(&cursor_x, &cursor_y, margin_x);
        }

        if (u != '\r') {
            ebook_draw_char(fb, cursor_x, cursor_y, u, 0x00);
            cursor_x += char_w;
        }
    }

    fclose(fd);

    char pg[24];
    snprintf(pg, sizeof(pg), "- %d -", page_num + 1);
    int pg_w = 0;
    const char* pp = pg;
    while (*pp) {
        uint16_t u = decode_utf8(&pp);
        if (!u) break;
        pg_w += (u < 128) ? 14 : 24;
    }
    draw_text_line(fb, (screen_w - pg_w) / 2, screen_h - 52, pg, 0x00);
}

static bool page_start_known(const EbookEngine* eng, int page)
{
    if (page == 0) {
        return eng->valid;
    }
    return eng->page_offsets[page] > 0;
}

static void ensure_page_cached(EbookEngine* eng, int page)
{
    if (!eng->valid || page < 0 || page >= EBOOK_MAX_PAGES) return;
    if (page_start_known(eng, page)) return;

    int from = 0;
    for (int p = page - 1; p >= 0; p--) {
        if (p == 0 || eng->page_offsets[p] > 0) {
            from = p;
            break;
        }
    }

    eng->cached_through = from;
    while (eng->cached_through < page) {
        int from = eng->cached_through;
        uint32_t next = paginate_page_at_offset(eng->path, eng->page_offsets[from]);
        if (next <= eng->page_offsets[from]) {
            break;
        }
        eng->page_offsets[from + 1] = next;
        eng->cached_through = from + 1;
    }
}

void ebook_engine_init(EbookEngine* eng)
{
    memset(eng, 0, sizeof(*eng));
    eng->cached_through = -1;
}

bool ebook_engine_open(EbookEngine* eng, const char* path)
{
    ebook_engine_close(eng);

    FILE* fd = fopen(path, "r");
    if (!fd) {
        ESP_LOGW(TAG, "Cannot open: %s", path);
        return false;
    }
    fseek(fd, 0, SEEK_END);
    long fsize = ftell(fd);
    fclose(fd);

    if (fsize <= 0) {
        ESP_LOGW(TAG, "Empty file: %s", path);
        return false;
    }

    strncpy(eng->path, path, sizeof(eng->path) - 1);
    eng->page_offsets[0] = 0;
    eng->cached_through = 0;
    eng->valid = true;
    ESP_LOGD(TAG, "Opened: %s (%ld bytes)", path, fsize);
    return true;
}

void ebook_engine_close(EbookEngine* eng)
{
    eng->valid = false;
    eng->path[0] = '\0';
    eng->cached_through = -1;
    if (s_render_scratch) {
        free(s_render_scratch);
        s_render_scratch = NULL;
        s_render_scratch_bytes = 0;
    }
}

void ebook_engine_restore_position(EbookEngine* eng, int page, uint32_t offset)
{
    if (page <= 0 || page >= EBOOK_MAX_PAGES || offset == 0) {
        return;
    }
    eng->page_offsets[page] = offset;
}

int ebook_engine_load_saved_page(const char* path, uint32_t* offset_out)
{
    nvs_handle_t h;
    if (nvs_open("ebook", NVS_READONLY, &h) != ESP_OK) {
        if (offset_out) *offset_out = 0;
        return 0;
    }

    char key[16];
    snprintf(key, sizeof(key), "p%08lx", (unsigned long)djb2_hash(path));

    int32_t page = 0;
    if (nvs_get_i32(h, key, &page) != ESP_OK || page < 0) {
        page = 0;
    }

    if (offset_out) {
        snprintf(key, sizeof(key), "o%08lx", (unsigned long)djb2_hash(path));
        uint32_t offset = 0;
        if (nvs_get_u32(h, key, &offset) != ESP_OK) {
            offset = 0;
        }
        *offset_out = offset;
    }

    nvs_close(h);
    return (int)page;
}

void ebook_engine_save_page(const char* path, int page, uint32_t offset)
{
    if (page < 0) page = 0;

    nvs_handle_t h;
    if (nvs_open("ebook", NVS_READWRITE, &h) != ESP_OK) return;

    char key[16];
    uint32_t hash = djb2_hash(path);
    snprintf(key, sizeof(key), "p%08lx", (unsigned long)hash);
    nvs_set_i32(h, key, page);
    snprintf(key, sizeof(key), "o%08lx", (unsigned long)hash);
    nvs_set_u32(h, key, offset);
    nvs_commit(h);
    nvs_close(h);
}

bool ebook_engine_can_next(const EbookEngine* eng, int page)
{
    if (!eng->valid || page < 0 || page >= EBOOK_MAX_PAGES) return false;

    EbookEngine* mut = (EbookEngine*)eng;
    ensure_page_cached(mut, page);
    if (!page_start_known(eng, page)) return false;

    uint32_t start = eng->page_offsets[page];
    if (page + 1 <= eng->cached_through && eng->page_offsets[page + 1] > start) {
        return true;
    }

    uint32_t next = paginate_page_at_offset(eng->path, start);
    if (next > start && page + 1 <= EBOOK_MAX_PAGES) {
        mut->page_offsets[page + 1] = next;
        if (mut->cached_through < page + 1) {
            mut->cached_through = page + 1;
        }
        return true;
    }
    return false;
}

bool ebook_engine_render_page(EbookEngine* eng, EpdiyHighlevelState* hl,
                              int page, const char* title, bool diff_merge,
                              bool* out_changed)
{
    if (out_changed) {
        *out_changed = true;
    }
    if (!eng->valid || page < 0 || page >= EBOOK_MAX_PAGES) return false;

    ensure_page_cached(eng, page);
    if (!page_start_known(eng, page)) return false;

    uint32_t start = eng->page_offsets[page];
    uint32_t end_pos;

    if (page + 1 <= eng->cached_through && eng->page_offsets[page + 1] > start) {
        end_pos = eng->page_offsets[page + 1];
    } else {
        end_pos = paginate_page_at_offset(eng->path, start);
        if (end_pos > start && page + 1 <= EBOOK_MAX_PAGES) {
            eng->page_offsets[page + 1] = end_pos;
            if (eng->cached_through < page + 1) {
                eng->cached_through = page + 1;
            }
        }
    }

    uint8_t* fb = epd_hl_get_framebuffer(hl);
    char header[96];
    const char* name = title ? title : eng->path;
    const char* slash = strrchr(name, '/');
    if (slash) name = slash + 1;
    strncpy(header, name, sizeof(header) - 1);
    header[sizeof(header) - 1] = '\0';

    if (diff_merge) {
        size_t fb_bytes = (size_t)(epd_width() / 2) * epd_height();
        uint8_t* scratch = get_render_scratch(fb_bytes);
        uint8_t* back = epd_hl_get_back_framebuffer(hl);
        if (scratch && back) {
            memcpy(fb, back, fb_bytes);
            render_page_at_offset(scratch, eng->path, start, end_pos, header, page);
            bool changed = merge_framebuffer_diff(fb, scratch, fb_bytes);
            if (out_changed) {
                *out_changed = changed;
            }
            return true;
        }
        ESP_LOGW(TAG, "Scratch alloc failed, fallback to full page draw");
    }

    render_page_at_offset(fb, eng->path, start, end_pos, header, page);
    if (out_changed) {
        *out_changed = true;
    }

    return true;
}
