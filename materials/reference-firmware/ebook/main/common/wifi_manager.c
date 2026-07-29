#include "wifi_manager.h"
#include "wifi_config.h"
#include "event_bus.h"
#include "sdkconfig.h"
#include <string.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

#define TAG "WIFI_MGR"

volatile int g_wifi_state = 0;
char g_wifi_ip[16] = "0.0.0.0";

static int s_last_notified_state = -1;
static bool s_sta_configured = false;

static void notify_wifi_changed_if_needed(void)
{
    if (g_wifi_state != s_last_notified_state) {
        s_last_notified_state = g_wifi_state;
        event_post(EVT_WIFI_CHANGED);
    }
}

static const char* disconnect_reason_hint(uint8_t reason)
{
    switch (reason) {
    case 201: return "找不到热点，检查SSID";
    case 202: return "密码错误";
    case 204: return "握手超时，检查密码或信号";
    case 205: return "连接失败";
    default:  return "请检查热点与密码";
    }
}

static void apply_sta_config(const char* ssid, const char* password)
{
    wifi_config_t wifi_config = {0};
    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    s_sta_configured = true;
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        if (!s_sta_configured) {
            ESP_LOGW(TAG, "STA started but no Wi-Fi config set");
            return;
        }
        if (g_wifi_state != 2) {
            g_wifi_state = 1;
            notify_wifi_changed_if_needed();
        }
        esp_err_t err = esp_wifi_connect();
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "esp_wifi_connect: %s", esp_err_to_name(err));
        }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t* disc = (wifi_event_sta_disconnected_t*)event_data;
        ESP_LOGW(TAG, "Disconnected, reason=%d — %s",
                 disc->reason, disconnect_reason_hint(disc->reason));

        bool was_connected = (g_wifi_state == 2);
        g_wifi_state = 0;
        strcpy(g_wifi_ip, "0.0.0.0");
        if (was_connected) {
            notify_wifi_changed_if_needed();
        }

        esp_err_t err = esp_wifi_connect();
        if (err != ESP_OK && err != ESP_ERR_WIFI_CONN) {
            ESP_LOGW(TAG, "reconnect: %s", esp_err_to_name(err));
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        g_wifi_state = 2;
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
        sprintf(g_wifi_ip, IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "Got IP: %s", g_wifi_ip);
        notify_wifi_changed_if_needed();
    }
}

void wifi_connect(const char* ssid, const char* password)
{
    ESP_LOGI(TAG, "Connecting to %s...", ssid);
    apply_sta_config(ssid, password);

    if (g_wifi_state != 2) {
        g_wifi_state = 1;
        notify_wifi_changed_if_needed();
    }

    esp_err_t err = esp_wifi_connect();
    if (err != ESP_OK && err != ESP_ERR_WIFI_CONN) {
        ESP_LOGW(TAG, "esp_wifi_connect: %s", esp_err_to_name(err));
    }
}

void wifi_save_credentials(const char* ssid, const char* password)
{
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err == ESP_OK) {
        nvs_set_str(my_handle, "wifi_ssid", ssid);
        nvs_set_str(my_handle, "wifi_pass", password);
        nvs_commit(my_handle);
        nvs_close(my_handle);
        ESP_LOGI(TAG, "Wi-Fi credentials saved to NVS.");
    }
}

static bool demo_wifi_configured(void)
{
#if defined(CONFIG_EBOOK_WIFI_SSID)
    if (strlen(CONFIG_EBOOK_WIFI_SSID) > 0) {
        return true;
    }
#endif
    return DEMO_WIFI_SSID[0] != '\0'
        && strcmp(DEMO_WIFI_SSID, "your_wifi_ssid") != 0;
}

static void get_demo_wifi_credentials(const char** ssid, const char** password)
{
#if defined(CONFIG_EBOOK_WIFI_SSID) && defined(CONFIG_EBOOK_WIFI_PASSWORD)
    if (strlen(CONFIG_EBOOK_WIFI_SSID) > 0) {
        *ssid = CONFIG_EBOOK_WIFI_SSID;
        *password = CONFIG_EBOOK_WIFI_PASSWORD;
        return;
    }
#endif
    *ssid = DEMO_WIFI_SSID;
    *password = DEMO_WIFI_PASSWORD;
}

void wifi_manager_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    if (demo_wifi_configured()) {
        const char* ssid;
        const char* password;
        get_demo_wifi_credentials(&ssid, &password);
        ESP_LOGI(TAG, "Using demo Wi-Fi: %s", ssid);
        apply_sta_config(ssid, password);
    } else {
        ESP_LOGW(TAG, "Wi-Fi not configured. Edit main/common/wifi_config.h");
    }

    ESP_ERROR_CHECK(esp_wifi_start());
}
