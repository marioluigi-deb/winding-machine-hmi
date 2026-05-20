#pragma once
#include "config.h"

void motion_init();
void motion_poll();

// Commands to motion controller
void motion_home_all();
void motion_home_axis(char axis);
void motion_move_to(float x, float y, float z, float feed);
void motion_jog(char axis, float distance);
void motion_spindle(int rpm, int direction);  // dir: 0=stop, 1=CW, 2=CCW
void motion_spindle_stop();
void motion_estop();
void motion_reset();
void motion_request_status();

// Winding commands
void motion_wind_load(const WindingProgram &prog);
void motion_wind_start();
void motion_wind_pause();
void motion_wind_resume();
void motion_wind_abort();
void motion_wind_speed(float rpm);

// Send raw config
void motion_set_config(const char *key, float value);

// State access
const MachineState &motion_get_state();
bool motion_is_connected();
unsigned long motion_last_rx_time();
