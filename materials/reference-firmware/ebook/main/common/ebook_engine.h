#ifndef EBOOK_ENGINE_H
#define EBOOK_ENGINE_H

#include "epd_highlevel.h"
#include <stdbool.h>
#include <stdint.h>

#define EBOOK_MAX_PAGES 512

typedef struct {
    char path[128];
    uint32_t page_offsets[EBOOK_MAX_PAGES + 1];
    int cached_through;
    bool valid;
} EbookEngine;

void ebook_engine_init(EbookEngine* eng);
bool ebook_engine_open(EbookEngine* eng, const char* path);
void ebook_engine_close(EbookEngine* eng);

int ebook_engine_load_saved_page(const char* path, uint32_t* offset_out);
void ebook_engine_save_page(const char* path, int page, uint32_t offset);
void ebook_engine_restore_position(EbookEngine* eng, int page, uint32_t offset);

bool ebook_engine_can_next(const EbookEngine* eng, int page);
bool ebook_engine_render_page(EbookEngine* eng, EpdiyHighlevelState* hl,
                              int page, const char* title, bool diff_merge,
                              bool* out_changed);

#endif /* EBOOK_ENGINE_H */
