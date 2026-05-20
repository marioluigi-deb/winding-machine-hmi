#pragma once
#include <stdint.h>

struct Stepper {
    uint8_t pin_step;
    uint8_t pin_dir;
    uint8_t pin_en;
    uint8_t pin_home;

    float   steps_per_mm;
    float   max_feed;       // mm/s
    float   accel;          // mm/s^2

    // ISR-safe integer state (no floats touched in ISR)
    volatile int32_t  position;        // current position in steps
    volatile int32_t  target;          // target position in steps
    volatile int32_t  i_velocity;      // current velocity in steps/s
    volatile int32_t  i_target_vel;    // requested velocity in steps/s
    volatile int32_t  i_accel_per_tick;// accel increment per ISR tick
    volatile int32_t  i_accum;         // step accumulator for timing
    volatile bool     moving;
    bool    homed;
    bool    dir_invert;
};

struct SpindleMotor {
    uint8_t pin_step;
    uint8_t pin_dir;
    uint8_t pin_en;

    float   steps_per_rev;

    // ISR-safe integer state
    volatile int32_t  i_rpm_x100;      // RPM × 100 fixed point
    volatile int32_t  i_target_rpm_x100;
    volatile int32_t  i_spr;           // steps per rev as int
    volatile int32_t  i_accum;         // step accumulator
    volatile int32_t  total_steps;
    volatile int      direction;       // 0=stop, 1=CW, 2=CCW
    volatile bool     running;

    float   rpm;           // readable by UI (updated from poll, not ISR)
    float   target_rpm;    // set by UI
};

#define STEP_ISR_HZ 20000

void stepper_init(Stepper &s);
void spindle_init(SpindleMotor &sp);

void stepper_isr_tick(Stepper &s);
void spindle_isr_tick(SpindleMotor &sp);

void stepper_move_to(Stepper &s, float pos_mm, float feed_mms);
void stepper_jog(Stepper &s, float dist_mm);
void stepper_stop(Stepper &s);
bool stepper_home(Stepper &s, float feed_mms);
float stepper_pos_mm(const Stepper &s);

void spindle_set_rpm(SpindleMotor &sp, float rpm);
void spindle_set_dir(SpindleMotor &sp, int dir);
void spindle_stop(SpindleMotor &sp);
int  spindle_turn_count(const SpindleMotor &sp);
void spindle_reset_turns(SpindleMotor &sp);
void spindle_poll_update(SpindleMotor &sp);
