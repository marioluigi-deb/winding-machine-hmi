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

    // Runtime state
    volatile int32_t  position;      // current position in steps
    volatile int32_t  target;        // target position in steps
    volatile float    velocity;      // current velocity in steps/s
    volatile float    target_vel;    // requested velocity
    volatile bool     moving;
    bool    homed;
    bool    dir_invert;
};

struct SpindleMotor {
    uint8_t pin_step;
    uint8_t pin_dir;
    uint8_t pin_en;

    float   steps_per_rev;

    // Runtime
    volatile float    rpm;
    volatile float    target_rpm;
    volatile int32_t  total_steps;   // total steps since wind start (for turn counting)
    volatile int      direction;     // 0=stop, 1=CW, 2=CCW
    volatile bool     running;
};

void stepper_init(Stepper &s);
void spindle_init(SpindleMotor &sp);

// Called from ISR timer — generates step pulses
void stepper_isr_tick(Stepper &s, float dt);
void spindle_isr_tick(SpindleMotor &sp, float dt);

// High-level commands
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
