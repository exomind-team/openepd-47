#include "image_storage.h"

#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_timer.h"
#include "sdkconfig.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define TAG "IMG_STOR"

/* SPIFFS 对象名上限（含 "photos/" 前缀与 ".raw" 后缀） */
#define SPIFFS_OBJ_PATH_MAX   (CONFIG_SPIFFS_OBJ_NAME_LEN - 1)
#define IMAGE_BASENAME_MAX    (SPIFFS_OBJ_PATH_MAX - 7)  /* strlen("photos/") */
#define IMAGE_NAME_BODY_MAX   (IMAGE_BASENAME_MAX - 4)   /* room for ".raw" */


static void ensure_dir(const char* dir)
{
    DIR* d = opendir(dir);
    if (d) {
        closedir(d);
        return;
    }

    if (mkdir(dir, 0777) == 0) {
        ESP_LOGI(TAG, "Created %s", dir);
        return;
    }

    if (errno != EEXIST) {
        ESP_LOGW(TAG, "mkdir %s failed: %s", dir, strerror(errno));
    }
}

static void migrate_legacy_album(void)
{
    struct stat st;
    if (stat(ALBUM_PATH, &st) != 0) {
        return;
    }

    ensure_dir(IMAGES_DIR);

    const char* dest = IMAGES_DIR "/migrated.raw";
    if (rename(ALBUM_PATH, dest) == 0) {
        ESP_LOGI(TAG, "Migrated legacy album to %s", dest);
        return;
    }

    ESP_LOGW(TAG, "Failed to migrate %s: %s", ALBUM_PATH, strerror(errno));
}

void image_storage_init(void)
{
    ensure_dir(IMAGES_DIR);
    migrate_legacy_album();
    ESP_LOGI(TAG, "Images dir ready: %s (max %d)", IMAGES_DIR, IMAGE_MAX_SLOTS);
}

static int cmp_paths(const void* a, const void* b)
{
    return strcmp((const char*)a, (const char*)b);
}

static void try_add(const char* name, char paths[][64], int* count, int max)
{
    size_t len = strlen(name);
    if (len <= 4 || strcasecmp(name + len - 4, ".raw") != 0) {
        return;
    }
    if (*count >= max) {
        return;
    }

    int n = snprintf(paths[*count], 64, "%s/%s", IMAGES_DIR, name);
    if (n <= 0 || n >= 64) {
        return;
    }
    (*count)++;
}

int image_storage_scan(char paths[][64], int max_count)
{
    int count = 0;

    DIR* d = opendir(IMAGES_DIR);
    if (d) {
        struct dirent* e;
        while ((e = readdir(d)) && count < max_count) {
            try_add(e->d_name, paths, &count, max_count);
        }
        closedir(d);
    }

    if (count > 1) {
        qsort(paths, (size_t)count, 64, cmp_paths);
    }

    ESP_LOGI(TAG, "Found %d image(s)", count);
    return count;
}

int image_storage_count(void)
{
    char paths[IMAGE_MAX_SLOTS][64];
    return image_storage_scan(paths, IMAGE_MAX_SLOTS);
}

static size_t spiffs_free_bytes(void)
{
    size_t total = 0;
    size_t used = 0;
    if (esp_spiffs_info(NULL, &total, &used) != ESP_OK) {
        return 0;
    }
    return (total > used) ? (total - used) : 0;
}

static bool has_free_space(int need_bytes)
{
    return spiffs_free_bytes() >= (size_t)need_bytes + 32768;
}

bool image_storage_can_upload_path(const char* dest_path)
{
    if (!dest_path || dest_path[0] == '\0') {
        return false;
    }

    struct stat st;
    bool exists = (stat(dest_path, &st) == 0);

    if (!exists && image_storage_count() >= IMAGE_MAX_SLOTS) {
        return false;
    }

    int need_bytes = exists ? (ALBUM_FILE_BYTES * 2) : ALBUM_FILE_BYTES;
    return has_free_space(need_bytes);
}

bool image_storage_can_upload(void)
{
    if (image_storage_count() >= IMAGE_MAX_SLOTS) {
        return false;
    }
    return has_free_space(ALBUM_FILE_BYTES);
}

static bool is_image_ext(const char* ext)
{
    return strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0
        || strcasecmp(ext, ".png") == 0 || strcasecmp(ext, ".gif") == 0
        || strcasecmp(ext, ".webp") == 0 || strcasecmp(ext, ".bmp") == 0
        || strcasecmp(ext, ".raw") == 0;
}

static void make_auto_filename(char* out, size_t out_len)
{
    static uint32_t seq;
    uint32_t id = (uint32_t)(esp_timer_get_time() / 1000) ^ (++seq);
    snprintf(out, out_len, "img_%08lx.raw", (unsigned long)id);
}

static void sanitize_filename(const char* in, char* out, size_t out_len)
{
    char base[IMAGE_NAME_BODY_MAX + 1];
    size_t j = 0;
    const char* p = in ? in : "photo";

    while (*p && j < IMAGE_NAME_BODY_MAX) {
        unsigned char c = (unsigned char)*p++;
        if (isalnum(c) || c == '_' || c == '-' || c == '.') {
            base[j++] = (char)c;
        }
    }
    base[j] = '\0';

    if (j == 0) {
        strncpy(base, "photo", sizeof(base) - 1);
        base[sizeof(base) - 1] = '\0';
    }

    char* dot = strrchr(base, '.');
    if (dot && dot != base && is_image_ext(dot)) {
        *dot = '\0';
    }

    if (strlen(base) > IMAGE_NAME_BODY_MAX) {
        base[IMAGE_NAME_BODY_MAX] = '\0';
    }

    int n = snprintf(out, out_len, "%s.raw", base);
    if (n <= 0 || (size_t)n >= out_len || (size_t)n > IMAGE_BASENAME_MAX) {
        make_auto_filename(out, out_len);
        if (in && in[0]) {
            ESP_LOGW(TAG, "Filename too long for SPIFFS, using %s (from %s)", out, in);
        }
    }
}

bool image_storage_make_upload_path(const char* upload_name, char* dest, size_t dest_len)
{
    char safe[IMAGE_BASENAME_MAX + 1];
    sanitize_filename(upload_name ? upload_name : "photo", safe, sizeof(safe));

    int n = snprintf(dest, dest_len, "%s/%s", IMAGES_DIR, safe);
    return n > 0 && (size_t)n < dest_len;
}

bool image_storage_make_path_from_name(const char* name, char* dest, size_t dest_len)
{
    return image_storage_make_upload_path(name, dest, dest_len);
}

static bool path_in_images_dir(const char* path)
{
    size_t dir_len = strlen(IMAGES_DIR);
    if (strncmp(path, IMAGES_DIR "/", dir_len + 1) != 0) {
        return false;
    }
    const char* rest = path + dir_len + 1;
    if (rest[0] == '\0' || strchr(rest, '/') != NULL) {
        return false;
    }
    return true;
}

bool image_storage_delete(const char* path)
{
    if (!path || !path_in_images_dir(path)) {
        return false;
    }

    if (remove(path) != 0) {
        ESP_LOGW(TAG, "remove %s failed: %s", path, strerror(errno));
        return false;
    }

    ESP_LOGI(TAG, "Deleted %s", path);
    return true;
}

bool image_storage_delete_by_name(const char* name)
{
    char path[128];
    if (!image_storage_make_path_from_name(name, path, sizeof(path))) {
        return false;
    }
    return image_storage_delete(path);
}

void image_storage_format_usage(char* buf, size_t buflen)
{
    if (buflen == 0) {
        return;
    }
    buf[0] = '\0';

    int count = image_storage_count();
    size_t total = 0;
    size_t used = 0;

    if (esp_spiffs_info(NULL, &total, &used) != ESP_OK || total == 0) {
        snprintf(buf, buflen, "图片: %d/%d · 存储: --", count, IMAGE_MAX_SLOTS);
        return;
    }

    snprintf(buf, buflen, "图片: %d/%d · 存储: %u/%u KB",
             count, IMAGE_MAX_SLOTS,
             (unsigned)((used + 1023) / 1024),
             (unsigned)((total + 1023) / 1024));
}

bool image_storage_prepare_space(int need_bytes, char* err, size_t err_len)
{
    esp_spiffs_gc(NULL, (size_t)need_bytes + 65536);

    size_t free_bytes = spiffs_free_bytes();
    ESP_LOGI(TAG, "SPIFFS free=%u need=%d", (unsigned)free_bytes, need_bytes);

    if (free_bytes < (size_t)need_bytes + 32768) {
        snprintf(err, err_len,
                 "存储空间不足(约剩%uKB，照片需%uKB)。请删除部分图片或书籍后重试",
                 (unsigned)(free_bytes / 1024), (unsigned)((need_bytes + 1023) / 1024));
        return false;
    }
    return true;
}
