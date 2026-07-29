#include "book_storage.h"
#include "ebook_engine.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <ctype.h>

#define TAG "BOOK_STOR"
#define SPIFFS_ROOT "/spiffs"

static void ensure_books_dir(void)
{
    DIR* d = opendir(BOOKS_DIR);
    if (d) {
        closedir(d);
        return;
    }

    if (mkdir(BOOKS_DIR, 0777) == 0) {
        ESP_LOGI(TAG, "Created %s", BOOKS_DIR);
        return;
    }

    if (errno != EEXIST) {
        ESP_LOGW(TAG, "mkdir %s failed: %s", BOOKS_DIR, strerror(errno));
    }
}

static void migrate_legacy_books(void)
{
    DIR* d = opendir(SPIFFS_ROOT);
    if (!d) {
        return;
    }

    struct dirent* e;
    while ((e = readdir(d)) != NULL) {
        const char* name = e->d_name;
        if (name[0] == '.' &&
            (name[1] == '\0' || (name[1] == '.' && name[2] == '\0'))) {
            continue;
        }
        if (strcmp(name, "books") == 0) {
            continue;
        }

        size_t len = strlen(name);
        if (len <= 4 || strcasecmp(name + len - 4, ".txt") != 0) {
            continue;
        }

        char src[64];
        char dest[64];
        snprintf(src, sizeof(src), "%s/%s", SPIFFS_ROOT, name);
        snprintf(dest, sizeof(dest), "%s/%s", BOOKS_DIR, name);

        struct stat st;
        if (stat(dest, &st) == 0) {
            if (remove(src) == 0) {
                ESP_LOGI(TAG, "Removed duplicate legacy book %s", src);
            } else {
                ESP_LOGW(TAG, "Failed to remove duplicate %s: %s",
                         src, strerror(errno));
            }
        } else if (rename(src, dest) == 0) {
            ESP_LOGI(TAG, "Migrated legacy book to %s", dest);
        } else {
            ESP_LOGW(TAG, "Failed to migrate %s: %s", src, strerror(errno));
        }
    }
    closedir(d);
}

void book_storage_init(void)
{
    ensure_books_dir();
    migrate_legacy_books();
    ESP_LOGI(TAG, "Books dir ready: %s", BOOKS_DIR);
}

static int cmp_paths(const void* a, const void* b)
{
    return strcmp((const char*)a, (const char*)b);
}

static void try_add(const char* name, char paths[][64], int* count, int max)
{
    size_t len = strlen(name);
    if (len <= 4 || strcasecmp(name + len - 4, ".txt") != 0) {
        return;
    }
    if (*count >= max) {
        return;
    }

    int n = snprintf(paths[*count], 64, "%s/%s", BOOKS_DIR, name);
    if (n <= 0 || n >= 64) {
        return;
    }
    (*count)++;
}

int book_storage_scan(char paths[][64], int max_count)
{
    int count = 0;

    DIR* d = opendir(BOOKS_DIR);
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

    ESP_LOGI(TAG, "Found %d book(s)", count);
    return count;
}

bool book_storage_has_progress(const char* path)
{
    return ebook_engine_load_saved_page(path, NULL) > 0;
}

void book_storage_format_usage(char* buf, size_t buflen)
{
    size_t total = 0;
    size_t used = 0;
    if (buflen == 0) return;
    buf[0] = '\0';

    if (esp_spiffs_info(NULL, &total, &used) != ESP_OK || total == 0) {
        snprintf(buf, buflen, "存储: --");
        return;
    }

    snprintf(buf, buflen, "存储: %u/%u KB",
             (unsigned)((used + 1023) / 1024),
             (unsigned)((total + 1023) / 1024));
}

static void sanitize_filename(const char* in, char* out, size_t out_len)
{
    size_t j = 0;
    const char* p = in ? in : "book.txt";

    while (*p && j + 1 < out_len) {
        unsigned char c = (unsigned char)*p++;
        if (isalnum(c) || c == '_' || c == '-' || c == '.') {
            out[j++] = (char)c;
        }
    }
    out[j] = '\0';

    if (j == 0) {
        strncpy(out, "book.txt", out_len - 1);
        out[out_len - 1] = '\0';
        return;
    }

    size_t len = strlen(out);
    if (len <= 4 || strcasecmp(out + len - 4, ".txt") != 0) {
        if (j + 4 < out_len) {
            strcat(out, ".txt");
        }
    }
}

bool book_storage_make_upload_path(const char* upload_name, char* dest, size_t dest_len)
{
    char safe[48];
    sanitize_filename(upload_name ? upload_name : "book.txt", safe, sizeof(safe));

    int n = snprintf(dest, dest_len, "%s/%s", BOOKS_DIR, safe);
    return n > 0 && (size_t)n < dest_len;
}
