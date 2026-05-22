#include "sound.h"
#include "config.h"
#include "motion.h"
#include <Arduino.h>

static MachineStatus prev_sound_status = MachineStatus::IDLE;

void sound_init() {
    Serial.println("SOUND: disabled (no compatible audio output)");
}

void sound_poll() {
    const MachineState &ms = motion_get_state();
    prev_sound_status = ms.status;
}

void sound_beep(int freq_hz, int duration_ms) {}
void sound_start_tone() {}
void sound_stop_tone() {}
void sound_complete_tone() {}
void sound_error_tone() {}
void sound_click() {}
