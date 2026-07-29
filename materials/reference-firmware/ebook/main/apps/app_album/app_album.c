#include "app_album.h"
#include "app_ebook.h"
#include "wifi_manager.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG "APP_ALBUM"
#define CHAR_SIZE 24

#define ALBUM_IMG_W  LOGICAL_SCREEN_W
#define ALBUM_IMG_H  LOGICAL_SCREEN_H

static void fb_put_pixel(uint8_t* fb, int px, int py, uint8_t color)
{
    int fb_w = epd_width();
    if (px < 0 || px >= fb_w || py < 0 || py >= epd_height()) {
        return;
    }
    uint8_t* ptr = &fb[py * fb_w / 2 + px / 2];
    if (px % 2) {
        *ptr = (*ptr & 0x0F) | (color & 0xF0);
    } else {
        *ptr = (*ptr & 0xF0) | (color >> 4);
    }
}

static uint8_t packed_gray(const uint8_t* data, int idx)
{
    uint8_t nibble = (idx & 1) ? (data[idx / 2] & 0x0F) : (data[idx / 2] >> 4);
    return nibble * 17;
}

static void draw_packed_portrait(uint8_t* fb, const uint8_t* packed)
{
    for (int ly = 0; ly < ALBUM_IMG_H; ly++) {
        for (int lx = 0; lx < ALBUM_IMG_W; lx++) {
            int idx = ly * ALBUM_IMG_W + lx;
            uint8_t gray = packed_gray(packed, idx);
            int px, py;
            logical_to_physical(lx, ly, &px, &py);
            fb_put_pixel(fb, px, py, gray);
        }
    }
}

static void draw_empty_hint(uint8_t* fb)
{
    char header[128];
    if (g_wifi_state == 2) {
        snprintf(header, sizeof(header), "浏览器访问 http://%s 上传照片", g_wifi_ip);
    } else {
        snprintf(header, sizeof(header), "Wi-Fi 连接中，请稍候...");
    }

    int header_x = 40;
    const char* hp = header;
    while (*hp != '\0') {
        uint16_t u = decode_utf8(&hp);
        if (u != 0) {
            ebook_draw_char(fb, header_x, LOGICAL_SCREEN_H / 2, u, 0x00);
            header_x += (u < 128) ? 14 : CHAR_SIZE;
        }
    }
}

bool album_draw_file(EpdiyHighlevelState* hl, const char* path)
{
    uint8_t* fb = epd_hl_get_framebuffer(hl);
    epd_fill_rect(epd_full_screen(), 0xFF, fb);

    if (!path) {
        draw_empty_hint(fb);
        return false;
    }

    FILE* fd = fopen(path, "rb");
    if (!fd) {
        ESP_LOGW(TAG, "No album at %s", path);
        draw_empty_hint(fb);
        return false;
    }

    fseek(fd, 0, SEEK_END);
    long fsize = ftell(fd);
    fseek(fd, 0, SEEK_SET);

    if (fsize < (long)ALBUM_FILE_BYTES) {
        ESP_LOGW(TAG, "Album file too small: %ld bytes (need %d)", fsize, ALBUM_FILE_BYTES);
        fclose(fd);
        draw_empty_hint(fb);
        return false;
    }

    uint8_t magic[ALBUM_MAGIC_LEN] = {0};
    fread(magic, 1, ALBUM_MAGIC_LEN, fd);

    if (memcmp(magic, ALBUM_MAGIC, ALBUM_MAGIC_LEN) != 0) {
        ESP_LOGW(TAG, "Album invalid magic at %s", path);
        fclose(fd);
        draw_empty_hint(fb);
        return false;
    }

    uint8_t* packed = heap_caps_malloc(ALBUM_PACKED_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!packed) {
        packed = malloc(ALBUM_PACKED_BYTES);
    }
    if (!packed) {
        ESP_LOGE(TAG, "malloc %d failed", ALBUM_PACKED_BYTES);
        fclose(fd);
        return false;
    }

    memset(packed, 0xFF, ALBUM_PACKED_BYTES);
    size_t got = fread(packed, 1, ALBUM_PACKED_BYTES, fd);
    fclose(fd);

    ESP_LOGI(TAG, "Album portrait %dx%d from %s (%u bytes)",
             ALBUM_IMG_W, ALBUM_IMG_H, path, (unsigned)got);
    draw_packed_portrait(fb, packed);

    free(packed);
    return true;
}

void album_draw_screen(EpdiyHighlevelState* hl)
{
    album_draw_file(hl, ALBUM_PATH);
}
