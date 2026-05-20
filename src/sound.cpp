#include "sound.h"
#include "config.h"
#include "motion.h"
#include <Arduino.h>

// Sound disabled until I2S audio is tested separately
// The CrowPanel P4 I2S pins (21,22,23) need specific init
// that may conflict with other peripherals.

static MachineStatus prev_sound_status = MachineStatus::IDLE;

void sound_init() {
    Serial.println("SOUND: stub (I2S deferred)");
}

void sound_poll() {
    const MachineState &ms = motion_get_state();
    if (ms.status != prev_sound_status) {
        prev_sound_status = ms.status;
    }
}

void sound_beep(int freq_hz, int duration_ms) {}
void sound_start_tone() {}
void sound_stop_tone() {}
void sound_complete_tone() {}
void sound_error_tone() {}
void sound_click() {}
