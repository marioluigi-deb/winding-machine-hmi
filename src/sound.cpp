#include "sound.h"
#include "config.h"
#include "motion.h"
#include <Arduino.h>

// P4 I2S DMA crashes when LCD RGB DMA is active (GDMA conflict).
// Use a piezo buzzer on GPIO 0 with LEDC PWM instead.
// Wire: GPIO 0 → buzzer + → buzzer - → GND
#define PIN_BUZZER 0

static bool sound_ready = false;
static unsigned long tone_end_ms = 0;
static MachineStatus prev_sound_status = MachineStatus::IDLE;

#define MAX_QUEUE 12
static struct { int freq; int dur; } snd_queue[MAX_QUEUE];
static volatile int q_head = 0, q_tail = 0;

static void enqueue(int freq, int dur) {
    int next = (q_tail + 1) % MAX_QUEUE;
    if (next != q_head) {
        snd_queue[q_tail] = {freq, dur};
        q_tail = next;
    }
}

void sound_init() {
    ledcAttach(PIN_BUZZER, 1000, 8);
    ledcWriteTone(PIN_BUZZER, 0);
    sound_ready = true;
    Serial.println("SOUND: piezo buzzer on GPIO 0");
}

void sound_poll() {
    // Handle tone end and queue
    if (tone_end_ms > 0 && millis() >= tone_end_ms) {
        // Current tone done — check queue
        if (q_head != q_tail) {
            int f = snd_queue[q_head].freq;
            int d = snd_queue[q_head].dur;
            q_head = (q_head + 1) % MAX_QUEUE;
            ledcWriteTone(PIN_BUZZER, f);
            tone_end_ms = millis() + d;
        } else {
            ledcWriteTone(PIN_BUZZER, 0);
            tone_end_ms = 0;
        }
    }

    // Auto state sounds
    const MachineState &ms = motion_get_state();
    MachineStatus cur = ms.status;
    if (cur != prev_sound_status) {
        if (cur == MachineStatus::WINDING && prev_sound_status != MachineStatus::PAUSED)
            sound_start_tone();
        if (prev_sound_status == MachineStatus::WINDING && cur == MachineStatus::IDLE)
            sound_complete_tone();
        if (cur == MachineStatus::ERROR || cur == MachineStatus::ESTOP)
            sound_error_tone();
        prev_sound_status = cur;
    }
}

static void start_tone(int freq, int dur) {
    if (!sound_ready) return;
    ledcWriteTone(PIN_BUZZER, freq);
    tone_end_ms = millis() + dur;
}

void sound_beep(int freq_hz, int duration_ms) {
    q_head = q_tail = 0;
    start_tone(freq_hz, duration_ms);
}

void sound_click() {
    q_head = q_tail = 0;
    start_tone(2000, 20);
}

void sound_start_tone() {
    q_head = q_tail = 0;
    enqueue(1200, 80);
    enqueue(1600, 120);
    start_tone(800, 80);
}

void sound_stop_tone() {
    q_head = q_tail = 0;
    start_tone(600, 200);
}

void sound_complete_tone() {
    q_head = q_tail = 0;
    enqueue(1200, 120);
    enqueue(1500, 120);
    enqueue(2000, 250);
    start_tone(1000, 120);
}

void sound_error_tone() {
    q_head = q_tail = 0;
    enqueue(200, 400);
    start_tone(400, 300);
}
