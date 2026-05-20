#include "notifications.h"
#include "config.h"
#include "motion.h"
#include "wifi_manager.h"
#include "storage.h"
#include <Arduino.h>
#include <HTTPClient.h>

static MachineStatus prev_status = MachineStatus::IDLE;
static unsigned long wind_start_ms = 0;

void notifications_init() {
    prev_status = motion_get_state().status;
}

static void send_webhook(const char *event, const char *message) {
    WiFiConfig &cfg = wifi_get_config();
    if (cfg.notify_url[0] == '\0') return;
    if (!wifi_is_connected()) return;

    const MachineState &ms = motion_get_state();
    char body[256];
    snprintf(body, sizeof(body),
        "{\"event\":\"%s\",\"message\":\"%s\",\"turn\":%d,\"layer\":%d,\"rpm\":%.0f}",
        event, message, ms.current_turn, ms.current_layer, ms.spindle_rpm);

    HTTPClient http;
    http.begin(cfg.notify_url);
    http.addHeader("Content-Type", "application/json");
    int code = http.POST((uint8_t *)body, strlen(body));
    if (code > 0) {
        Serial.printf("NOTIFY: %s -> %d\n", event, code);
    } else {
        Serial.printf("NOTIFY: %s failed: %s\n", event, http.errorToString(code).c_str());
    }
    http.end();
}

void notifications_check() {
    const MachineState &ms = motion_get_state();
    MachineStatus cur = ms.status;
    WiFiConfig &cfg = wifi_get_config();

    if (cur == prev_status) {
        if (cur == MachineStatus::WINDING && wind_start_ms == 0)
            wind_start_ms = millis();
        return;
    }

    // State transition
    if (prev_status == MachineStatus::WINDING && cur == MachineStatus::IDLE) {
        if (cfg.notify_on_complete) {
            char msg[64];
            unsigned long elapsed = (millis() - wind_start_ms) / 1000;
            snprintf(msg, sizeof(msg), "Winding complete. %d turns in %lus", ms.current_turn, elapsed);
            send_webhook("winding_complete", msg);
        }
        wind_start_ms = 0;
    }

    if (cur == MachineStatus::ERROR && cfg.notify_on_error) {
        send_webhook("error", ms.error_msg);
    }

    if (cur == MachineStatus::ESTOP && cfg.notify_on_error) {
        send_webhook("estop", "E-STOP triggered");
    }

    if (cur == MachineStatus::WINDING) {
        wind_start_ms = millis();
    }

    prev_status = cur;
}
