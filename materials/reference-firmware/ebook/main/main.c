/* Simple firmware for a ESP32 displaying a static image on an EPaper Screen */

#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "dragon.h"
#include "epd_highlevel.h"
#include "epdiy.h"

#include "wifi_manager.h"
#include "key_manager.h"
#include "web_server.h"
#include "ui_manager.h"
#include "ui_theme.h"
#include "my_waveform.h"

EpdiyHighlevelState hl;

#ifdef CONFIG_IDF_TARGET_ESP32
#define DEMO_BOARD epd_board_v6
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
#define DEMO_BOARD epd_board_v7
#endif

/* ED047TC2 4.7" 屏 VCOM（mV），可按上板效果在 750–1200 范围微调 */
#define EPD_VCOM_MV  1200

void idf_setup(void)
{
    epd_init(&DEMO_BOARD, &ED047TC2_1216, EPD_LUT_1K);
    epd_set_vcom(EPD_VCOM_MV);
    epd_set_rotation(EPD_ROT_LANDSCAPE);
    hl = epd_hl_init(&MY_WAVEFORM);

    spiffs_init();
    wifi_manager_init();
    key_manager_init();
}

#ifndef ARDUINO_ARCH_ESP32
void app_main(void)
{
    ESP_LOGI("MAIN", "System booting...");
    idf_setup();

    if (ui_manager_check_factory_boot(&hl)) {
        ESP_LOGI("MAIN", "Factory test mode — normal UI disabled");
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    ui_manager_init();

    /* 墨水屏断电后仍保留上一帧；开机先 GC16 全刷白，再画 UI */
    ESP_LOGI("MAIN", "Boot panel clear (remove previous image ghost)");
    ui_panel_clear(&hl);

    bool webserver_started = false;

    while (1) {
        if (g_wifi_state == 2 && !webserver_started) {
            start_webserver();
            webserver_started = true;
        }

        ui_manager_tick(&hl);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
#endif
