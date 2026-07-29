#include "event_bus.h"

static volatile AppEvent_t s_pending = EVT_NONE;

void event_post(AppEvent_t e)
{
    if (e != EVT_NONE) {
        s_pending = e;
    }
}

AppEvent_t event_poll(void)
{
    AppEvent_t e = s_pending;
    s_pending = EVT_NONE;
    return e;
}
