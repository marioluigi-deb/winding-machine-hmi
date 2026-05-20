#include "wifi_manager.h"
#include "storage.h"
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include "esp32-hal-hosted.h"

// Embedded C6 slave firmware (v2.12.3 matching ESP-IDF 5.5)
extern const uint8_t c6_slave_bin_start[] asm("_binary_c6_slave_bin_start");
extern const uint8_t c6_slave_bin_end[] asm("_binary_c6_slave_bin_end");

static WiFiConfig cfg;
static WiFiState state = WiFiState::OFF;
static unsigned long connect_start = 0;
static unsigned long last_reconnect = 0;
static char ip_str[20] = "0.0.0.0";

WiFiConfig &wifi_get_config() { return cfg; }

static void wifi_event_handler(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.printf("WIFI EVENT: %d\n", event);
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_START:
            Serial.println("WIFI: STA started"); break;
        case ARDUINO_EVENT_WIFI_STA_STOP:
            Serial.println("WIFI: STA stopped"); break;
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            Serial.println("WIFI: STA connected to AP"); break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            Serial.printf("WIFI: STA disconnected, reason=%d\n", info.wifi_sta_disconnected.reason); break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            Serial.printf("WIFI: got IP %s\n", IPAddress(info.got_ip.ip_info.ip.addr).toString().c_str()); break;
        case ARDUINO_EVENT_WIFI_SCAN_DONE:
            Serial.printf("WIFI: scan done, found=%d\n", info.wifi_scan_done.number); break;
        default: break;
    }
}

void wifi_init() {
    WiFi.onEvent(wifi_event_handler);
    storage_load_wifi(cfg);

    if (cfg.ssid[0] == '\0') {
        strlcpy(cfg.ssid, "2030", sizeof(cfg.ssid));
        strlcpy(cfg.password, "Aple0123", sizeof(cfg.password));
        strlcpy(cfg.hostname, "winder", sizeof(cfg.hostname));
        cfg.auto_connect = true;
        cfg.notify_on_complete = true;
        cfg.notify_on_error = true;
        storage_save_wifi(cfg);
    }

    Serial.printf("WIFI: hostname=%s auto=%d\n", cfg.hostname, cfg.auto_connect);

    // CrowPanel P4 SDIO pins to C6 WiFi module — bare minimum init
    WiFi.setPins(53, 54, 52, 51, 50, 49, 20);
    WiFi.setHostname(cfg.hostname);

    if (cfg.auto_connect && cfg.ssid[0] != '\0') {
        wifi_connect(cfg.ssid, cfg.password);
    } else {
        wifi_start_ap();
    }
}

void wifi_connect(const char *ssid, const char *pass) {
    Serial.printf("WIFI: connecting to '%s'\n", ssid);
    WiFi.begin(ssid, pass);
    state = WiFiState::CONNECTING;
    connect_start = millis();
}

void wifi_disconnect() {
    WiFi.disconnect();
    state = WiFiState::DISCONNECTED;
}

void wifi_start_ap() {
    Serial.println("WIFI: starting AP 'WinderSetup'");
    WiFi.mode(WIFI_AP);
    WiFi.softAP("WinderSetup", "winder123");
    IPAddress ip = WiFi.softAPIP();
    snprintf(ip_str, sizeof(ip_str), "%s", ip.toString().c_str());
    state = WiFiState::AP_ACTIVE;
    MDNS.begin(cfg.hostname);
    Serial.printf("WIFI: AP active at %s\n", ip_str);
}

void wifi_poll() {
    if (state == WiFiState::CONNECTING) {
        wl_status_t ws = WiFi.status();
        if (ws == WL_CONNECTED) {
            state = WiFiState::CONNECTED;
            IPAddress ip = WiFi.localIP();
            snprintf(ip_str, sizeof(ip_str), "%s", ip.toString().c_str());
            MDNS.begin(cfg.hostname);
            Serial.printf("WIFI: connected, IP=%s\n", ip_str);
        } else if (millis() - connect_start > 30000) {
            Serial.printf("WIFI: connect timeout (status=%d), starting AP\n", ws);
            wifi_start_ap();
        } else if ((millis() - connect_start) % 5000 < 50) {
            Serial.printf("WIFI: still connecting... status=%d\n", ws);
        }
    } else if (state == WiFiState::CONNECTED) {
        if (WiFi.status() != WL_CONNECTED) {
            state = WiFiState::DISCONNECTED;
            Serial.println("WIFI: disconnected");
        }
    } else if (state == WiFiState::DISCONNECTED) {
        if (cfg.auto_connect && cfg.ssid[0] != '\0' && millis() - last_reconnect > 10000) {
            last_reconnect = millis();
            wifi_connect(cfg.ssid, cfg.password);
        }
    }
}

WiFiState wifi_get_state() { return state; }

const char *wifi_get_ip() { return ip_str; }

const char *wifi_get_ssid() {
    if (state == WiFiState::AP_ACTIVE) return "WinderSetup (AP)";
    return cfg.ssid;
}

int wifi_get_rssi() {
    if (state == WiFiState::CONNECTED) return WiFi.RSSI();
    return 0;
}

bool wifi_is_connected() {
    return state == WiFiState::CONNECTED || state == WiFiState::AP_ACTIVE;
}
