#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

extern volatile int g_wifi_state;
extern char g_wifi_ip[16];

void wifi_manager_init(void);
void wifi_connect(const char* ssid, const char* password);
void wifi_save_credentials(const char* ssid, const char* password);

#endif /* WIFI_MANAGER_H */
