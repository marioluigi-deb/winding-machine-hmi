#include <Arduino.h>
#include "config.h"
#include "display.h"
#include "motion.h"
#include "storage.h"
#include "wifi_manager.h"
#include "web_server.h"
#include "notifications.h"
#include "sound.h"
#include "ui/ui.h"

static unsigned long last_ui_update = 0;

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("=== Winding Machine HMI v2.0 ===");

    storage_init();
    display_init();
    ui_init();
    motion_init();
    wifi_init();
    web_server_init();
    notifications_init();
    sound_init();
    display_backlight_on();

    Serial.println("=== READY ===");
}

void loop() {
    display_tick();
    motion_poll();

    if (millis() - last_ui_update > 33) {
        last_ui_update = millis();
        ui_update();
    }

    sound_poll();
    wifi_poll();
    web_server_poll();
    notifications_check();
}
