#include "motion.h"
#include "stepper.h"
#include "winder.h"
#include <Arduino.h>

// Pin assignments from config.h:
// X axis (traverse): GPIO 47 step, 48 dir
// Y axis (radial):   GPIO 29 step, 30 dir
// Spindle:           GPIO 31 step, 32 dir
// E-stop input:      GPIO 26 (active LOW, internal pullup)
// DM542 ENA tied to GND (always enabled) — no enable pins needed

// ============================================================
// Hardware state
// ============================================================
static Stepper axis_x, axis_y;
static SpindleMotor spindle_motor;
static Winder winder;
static MachineState state = {};
static bool estop_active = false;

// 20kHz step generation timer
static hw_timer_t *step_timer = nullptr;

static void IRAM_ATTR step_timer_isr() {
    if (estop_active) return;
    stepper_isr_tick(axis_x);
    stepper_isr_tick(axis_y);
    spindle_isr_tick(spindle_motor);
}

static void check_limits() {
    if (digitalRead(PIN_ESTOP) == LOW && !estop_active) {
        motion_estop();
    }
}

static void update_state() {
    state.pos_x = stepper_pos_mm(axis_x);
    state.pos_y = stepper_pos_mm(axis_y);
    state.pos_z = 0;
    state.spindle_rpm = spindle_motor.rpm;
    state.current_turn = winder.status().current_turn;
    state.current_layer = winder.status().current_layer;
    state.homed_x = axis_x.homed;
    state.homed_y = axis_y.homed;
    state.homed_z = true;

    if (estop_active) {
        state.status = MachineStatus::ESTOP;
    } else {
        auto ws = winder.status().state;
        if (ws == WinderState::WINDING)       state.status = MachineStatus::WINDING;
        else if (ws == WinderState::PAUSED)    state.status = MachineStatus::PAUSED;
        else if (ws == WinderState::ERROR)     state.status = MachineStatus::ERROR;
        else if (axis_x.moving || axis_y.moving) state.status = MachineStatus::MOVING;
        else                                   state.status = MachineStatus::IDLE;
    }
}

// ============================================================
// Public API (called by UI)
// ============================================================

void motion_init() {
    Serial.println("MOTION: init steppers");

    axis_x = {};
    axis_x.pin_step = PIN_X_STEP;
    axis_x.pin_dir = PIN_X_DIR;
    axis_x.pin_en = 0xFF;
    axis_x.pin_home = 0xFF;
    axis_x.steps_per_mm = DEFAULT_STEPS_PER_MM_X;
    axis_x.max_feed = DEFAULT_MAX_FEED_X / 60.0f;
    axis_x.accel = DEFAULT_ACCEL_X;
    stepper_init(axis_x);

    axis_y = {};
    axis_y.pin_step = PIN_Y_STEP;
    axis_y.pin_dir = PIN_Y_DIR;
    axis_y.pin_en = 0xFF;
    axis_y.pin_home = 0xFF;
    axis_y.steps_per_mm = DEFAULT_STEPS_PER_MM_Y;
    axis_y.max_feed = DEFAULT_MAX_FEED_Y / 60.0f;
    axis_y.accel = DEFAULT_ACCEL_Y;
    stepper_init(axis_y);

    spindle_motor = {};
    spindle_motor.pin_step = PIN_SP_STEP;
    spindle_motor.pin_dir = PIN_SP_DIR;
    spindle_motor.pin_en = 0xFF;
    spindle_motor.steps_per_rev = DEFAULT_STEPS_PER_REV;
    spindle_init(spindle_motor);

    // E-stop input — active LOW with internal pullup
    pinMode(PIN_ESTOP, INPUT_PULLUP);

    winder.init(&axis_x, &axis_y, nullptr, &spindle_motor);
    state.status = MachineStatus::IDLE;

    step_timer = timerBegin(1000000);
    timerAttachInterrupt(step_timer, &step_timer_isr);
    timerAlarm(step_timer, 1000000 / STEP_ISR_HZ, true, 0);

    Serial.println("MOTION: ready — X(47,48) Y(29,30) SP(31,32) ESTOP(26) 20kHz");
}

void motion_poll() {
    check_limits();
    winder.update();
    spindle_poll_update(spindle_motor);
    update_state();
}

void motion_home_all() {
    stepper_home(axis_x, 5.0f);
    stepper_home(axis_y, 5.0f);
}

void motion_home_axis(char axis) {
    switch (axis) {
        case 'X': stepper_home(axis_x, 5.0f); break;
        case 'Y': stepper_home(axis_y, 5.0f); break;
    }
}

void motion_move_to(float x, float y, float z, float feed) {
    float feed_mms = feed / 60.0f;
    stepper_move_to(axis_x, x, feed_mms);
    stepper_move_to(axis_y, y, feed_mms);
}

void motion_jog(char axis, float distance) {
    switch (axis) {
        case 'X': stepper_jog(axis_x, distance); break;
        case 'Y': stepper_jog(axis_y, distance); break;
    }
}

void motion_spindle(int rpm, int direction) {
    spindle_set_dir(spindle_motor, direction);
    spindle_set_rpm(spindle_motor, (float)rpm);
}

void motion_spindle_stop() {
    spindle_stop(spindle_motor);
}

void motion_estop() {
    estop_active = true;
    spindle_stop(spindle_motor);
    stepper_stop(axis_x);
    stepper_stop(axis_y);
    winder.abort();
    state.status = MachineStatus::ESTOP;
}

void motion_reset() {
    estop_active = false;
    state.status = MachineStatus::IDLE;
    state.error_msg[0] = '\0';
}

void motion_request_status() {
    update_state();
}

void motion_wind_load(const WindingProgram &prog) {
    WinderParams wp = {};
    wp.total_turns = prog.total_turns;
    wp.num_layers = prog.num_layers;
    wp.bobbin_width_mm = prog.bobbin_width_mm;
    wp.bobbin_id_mm = prog.bobbin_id_mm;
    wp.wire_od_mm = prog.wire_od_mm;
    wp.speed_rpm = prog.speed_rpm;
    wp.direction_cw = prog.direction_cw;
    wp.layer_step_mm = prog.layer_step_mm;
    winder.load_program(wp);
}

void motion_wind_start() {
    winder.start();
}

void motion_wind_pause() {
    winder.pause();
}

void motion_wind_resume() {
    winder.resume();
}

void motion_wind_abort() {
    winder.abort();
}

void motion_wind_speed(float rpm) {
    winder.set_speed(rpm);
}

void motion_set_config(const char *key, float value) {
    if (strcmp(key, "SPM_X") == 0) axis_x.steps_per_mm = value;
    else if (strcmp(key, "SPM_Y") == 0) axis_y.steps_per_mm = value;
    else if (strcmp(key, "SPR") == 0)   spindle_motor.steps_per_rev = value;
    else if (strcmp(key, "MF_X") == 0)  axis_x.max_feed = value / 60.0f;
    else if (strcmp(key, "MF_Y") == 0)  axis_y.max_feed = value / 60.0f;
    else if (strcmp(key, "ACC_X") == 0) axis_x.accel = value;
    else if (strcmp(key, "ACC_Y") == 0) axis_y.accel = value;
}

const MachineState &motion_get_state() {
    return state;
}

bool motion_is_connected() {
    return true;  // always connected — on-chip
}

unsigned long motion_last_rx_time() {
    return millis();
}
