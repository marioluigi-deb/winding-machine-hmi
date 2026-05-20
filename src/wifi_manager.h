#pragma once
#include "config.h"

enum class WiFiState : uint8_t {
    OFF, CONNECTING, CONNECTED, AP_ACTIVE, DISCONNECTED
};

void wifi_init();
void wifi_poll();
void wifi_connect(const char *ssid, const char *pass);
void wifi_disconnect();
void wifi_start_ap();
WiFiState wifi_get_state();
const char *wifi_get_ip();
const char *wifi_get_ssid();
int wifi_get_rssi();
bool wifi_is_connected();
WiFiConfig &wifi_get_config();
