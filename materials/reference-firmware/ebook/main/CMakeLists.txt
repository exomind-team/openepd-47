set(app_sources
    "main.c"
    "factor_test.c"
    "common/wifi_manager.c"
    "common/event_bus.c"
    "common/ebook_engine.c"
    "common/book_storage.c"
    "common/image_storage.c"
    "common/key_manager.c"
    "common/web_server.c"
    "common/ui_theme.c"
    "common/ui_manager.c"
    "apps/app_ebook/app_ebook.c"
    "apps/app_album/app_album.c"
    "waveforms/my_waveform.c"
)

idf_component_register(
    SRCS ${app_sources}
    INCLUDE_DIRS "." "common" "waveforms" "apps/app_ebook" "apps/app_album"
    REQUIRES epdiy nvs_flash esp_wifi esp_event lwip esp_netif spiffs esp_http_server
)
