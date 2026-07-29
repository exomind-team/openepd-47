#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "epd_highlevel.h"

typedef enum {
    UI_STATE_HOME,
    UI_STATE_FILE_LIST,
    UI_STATE_READER_OPENING,
    UI_STATE_READER,
    UI_STATE_ALBUM_LIST,
    UI_STATE_ALBUM_VIEW,
    UI_STATE_ALBUM_DELETE,
} UIState_t;

typedef enum {
    UI_KEY_NONE = 0,
    UI_KEY_UP,
    UI_KEY_DOWN,
    UI_KEY_CONFIRM,
    UI_KEY_CANCEL,
} UIKey_t;

extern UIState_t g_ui_state;

void ui_manager_init(void);
void ui_manager_tick(EpdiyHighlevelState* hl);
bool ui_manager_check_factory_boot(EpdiyHighlevelState* hl);

#endif /* UI_MANAGER_H */
