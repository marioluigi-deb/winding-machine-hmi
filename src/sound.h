#pragma once

void sound_init();
void sound_poll();
void sound_beep(int freq_hz, int duration_ms);
void sound_start_tone();
void sound_stop_tone();
void sound_complete_tone();
void sound_error_tone();
void sound_click();
