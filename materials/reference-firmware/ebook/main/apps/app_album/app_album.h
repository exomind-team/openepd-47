#ifndef APP_ALBUM_H
#define APP_ALBUM_H

#include "epd_highlevel.h"
#include "app_ebook.h"
#include <stdbool.h>

#define ALBUM_PATH         "/spiffs/album.raw"
#define ALBUM_MAGIC        "EI02"
#define ALBUM_MAGIC_LEN    4
#define ALBUM_PACKED_BYTES ((LOGICAL_SCREEN_W * LOGICAL_SCREEN_H) / 2)
#define ALBUM_FILE_BYTES   (ALBUM_MAGIC_LEN + ALBUM_PACKED_BYTES)

bool album_draw_file(EpdiyHighlevelState* hl, const char* path);
void album_draw_screen(EpdiyHighlevelState* hl);

#endif /* APP_ALBUM_H */
