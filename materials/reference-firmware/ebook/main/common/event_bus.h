#ifndef EVENT_BUS_H
#define EVENT_BUS_H

typedef enum {
    EVT_NONE = 0,
    EVT_BOOK_UPLOADED,
    EVT_ALBUM_UPLOADED,
    EVT_IMAGE_CHANGED,
    EVT_WIFI_CHANGED,
} AppEvent_t;

void event_post(AppEvent_t e);
AppEvent_t event_poll(void);

#endif /* EVENT_BUS_H */
