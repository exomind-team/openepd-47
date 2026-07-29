#ifndef BOOK_STORAGE_H
#define BOOK_STORAGE_H

#include <stdbool.h>
#include <stddef.h>

#define BOOKS_DIR "/spiffs/books"

void book_storage_init(void);
int book_storage_scan(char paths[][64], int max_count);
bool book_storage_has_progress(const char* path);
void book_storage_format_usage(char* buf, size_t buflen);
bool book_storage_make_upload_path(const char* upload_name, char* dest, size_t dest_len);

#endif /* BOOK_STORAGE_H */
