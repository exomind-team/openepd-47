#ifndef IMAGE_STORAGE_H
#define IMAGE_STORAGE_H

#include <stdbool.h>
#include <stddef.h>

#include "app_album.h"

#ifndef IMAGE_MAX_SLOTS
#define IMAGE_MAX_SLOTS  10
#endif

#define IMAGES_DIR  "/spiffs/photos"

void image_storage_init(void);
int image_storage_scan(char paths[][64], int max_count);
int image_storage_count(void);
bool image_storage_can_upload(void);
bool image_storage_can_upload_path(const char* dest_path);
bool image_storage_make_upload_path(const char* upload_name, char* dest, size_t dest_len);
bool image_storage_make_path_from_name(const char* name, char* dest, size_t dest_len);
bool image_storage_delete(const char* path);
bool image_storage_delete_by_name(const char* name);
void image_storage_format_usage(char* buf, size_t buflen);
bool image_storage_prepare_space(int need_bytes, char* err, size_t err_len);

#endif /* IMAGE_STORAGE_H */
