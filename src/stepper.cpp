#include "stepper.h"
#include <Arduino.h>

void stepper_init(Stepper &s) {
    if (s.pin_step == 0xFF) return;
    pinMode(s.pin_step, OUTPUT);
    pinMode(s.pin_dir, OUTPUT);
    if (s.pin_en != 0xFF) {
        pinMode(s.pin_en, OUTPUT);
        digitalWrite(s.pin_en, HIGH);
    }
    if (s.pin_home != 0xFF)
        pinMode(s.pin_home, INPUT_PULLUP);
    digitalWrite(s.pin_step, LOW);
    s.position = 0;
    s.target = 0;
    s.i_velocity = 0;
    s.i_target_vel = 0;
    s.i_accel_per_tick = 1;
    s.i_accum = 0;
    s.moving = false;
    s.homed = false;
    s.dir_invert = false;
}

void spindle_init(SpindleMotor &sp) {
    if (sp.pin_step == 0xFF) return;
    pinMode(sp.pin_step, OUTPUT);
    pinMode(sp.pin_dir, OUTPUT);
    pinMode(sp.pin_en, OUTPUT);
    digitalWrite(sp.pin_en, HIGH);
    digitalWrite(sp.pin_step, LOW);
    sp.rpm = 0;
    sp.target_rpm = 0;
    sp.i_rpm_x100 = 0;
    sp.i_target_rpm_x100 = 0;
    sp.i_spr = (int32_t)sp.steps_per_rev;
    sp.i_accum = 0;
    sp.total_steps = 0;
    sp.direction = 0;
    sp.running = false;
}

// ============================================================
// ISR functions — PURE INTEGER, zero float/double
// ============================================================

void IRAM_ATTR stepper_isr_tick(Stepper &s) {
    if (!s.moving) return;

    int32_t dist = s.target - s.position;
    if (dist == 0) {
        s.moving = false;
        s.i_velocity = 0;
        return;
    }

    int32_t abs_vel = s.i_velocity > 0 ? s.i_velocity : -s.i_velocity;
    int32_t abs_dist = dist > 0 ? dist : -dist;
    int32_t apt = s.i_accel_per_tick;

    // Decel distance = v^2 / (2 * a_per_tick * ISR_HZ)
    int32_t accel_sps = apt * STEP_ISR_HZ;
    int32_t decel_steps = 0;
    if (accel_sps > 0) {
        decel_steps = (int32_t)((int64_t)abs_vel * abs_vel / (2 * accel_sps));
    }

    if (abs_dist <= decel_steps + 1) {
        abs_vel -= apt;
        if (abs_vel < 10) abs_vel = 10;
    } else if (abs_vel < s.i_target_vel) {
        abs_vel += apt;
        if (abs_vel > s.i_target_vel) abs_vel = s.i_target_vel;
    }

    s.i_velocity = (dist > 0) ? abs_vel : -abs_vel;

    // Accumulator-based step emission
    s.i_accum += abs_vel;
    if (s.i_accum >= STEP_ISR_HZ) {
        s.i_accum -= STEP_ISR_HZ;
        digitalWrite(s.pin_dir, ((dist > 0) ^ s.dir_invert) ? HIGH : LOW);
        digitalWrite(s.pin_step, HIGH);
        digitalWrite(s.pin_step, LOW);
        s.position += (dist > 0) ? 1 : -1;
    }
}

void IRAM_ATTR spindle_isr_tick(SpindleMotor &sp) {
    if (!sp.running || sp.direction == 0) return;

    // Ramp RPM: 1 unit/tick = 0.01 RPM/tick = 200 RPM/s
    int32_t diff = sp.i_target_rpm_x100 - sp.i_rpm_x100;
    if (diff > 1) sp.i_rpm_x100 += 1;
    else if (diff < -1) sp.i_rpm_x100 -= 1;
    else sp.i_rpm_x100 = sp.i_target_rpm_x100;

    if (sp.i_rpm_x100 < 100) {
        sp.running = false;
        return;
    }

    // step_freq = (rpm/60) * steps_per_rev
    // freq_x10 = rpm_x100 * spr / 600
    int32_t freq_x10 = (int32_t)((int64_t)sp.i_rpm_x100 * sp.i_spr / 600);

    sp.i_accum += freq_x10;
    if (sp.i_accum >= (STEP_ISR_HZ * 10)) {
        sp.i_accum -= (STEP_ISR_HZ * 10);
        digitalWrite(sp.pin_dir, (sp.direction == 1) ? HIGH : LOW);
        digitalWrite(sp.pin_step, HIGH);
        digitalWrite(sp.pin_step, LOW);
        sp.total_steps++;
    }
}

// ============================================================
// Non-ISR functions (float OK)
// ============================================================

void stepper_move_to(Stepper &s, float pos_mm, float feed_mms) {
    s.target = (int32_t)(pos_mm * s.steps_per_mm);
    float vel = feed_mms * s.steps_per_mm;
    float max_vel = s.max_feed * s.steps_per_mm;
    if (vel > max_vel) vel = max_vel;
    s.i_target_vel = (int32_t)vel;
    s.i_accel_per_tick = (int32_t)(s.accel * s.steps_per_mm / STEP_ISR_HZ);
    if (s.i_accel_per_tick < 1) s.i_accel_per_tick = 1;
    s.moving = true;
    digitalWrite(s.pin_en, LOW);
}

void stepper_jog(Stepper &s, float dist_mm) {
    float current_mm = (float)s.position / s.steps_per_mm;
    stepper_move_to(s, current_mm + dist_mm, s.max_feed * 0.5f);
}

void stepper_stop(Stepper &s) {
    s.target = s.position;
    s.moving = false;
    s.i_velocity = 0;
}

bool stepper_home(Stepper &s, float feed_mms) {
    if (s.pin_home == 0xFF) return false;
    s.target = s.position - (int32_t)(300.0f * s.steps_per_mm);
    s.i_target_vel = (int32_t)(feed_mms * s.steps_per_mm);
    s.i_accel_per_tick = (int32_t)(s.accel * s.steps_per_mm / STEP_ISR_HZ);
    if (s.i_accel_per_tick < 1) s.i_accel_per_tick = 1;
    s.moving = true;
    digitalWrite(s.pin_en, LOW);
    return true;
}

float stepper_pos_mm(const Stepper &s) {
    return (float)s.position / s.steps_per_mm;
}

void spindle_set_rpm(SpindleMotor &sp, float rpm) {
    sp.target_rpm = rpm;
    sp.i_target_rpm_x100 = (int32_t)(rpm * 100);
    sp.i_spr = (int32_t)sp.steps_per_rev;
    if (rpm > 0 && sp.direction != 0) {
        sp.running = true;
        digitalWrite(sp.pin_en, LOW);
    }
}

void spindle_set_dir(SpindleMotor &sp, int dir) {
    sp.direction = dir;
    if (dir == 0) {
        sp.target_rpm = 0;
        sp.i_target_rpm_x100 = 0;
    } else {
        sp.running = true;
        digitalWrite(sp.pin_en, LOW);
    }
}

void spindle_stop(SpindleMotor &sp) {
    sp.target_rpm = 0;
    sp.i_target_rpm_x100 = 0;
    sp.direction = 0;
}

int spindle_turn_count(const SpindleMotor &sp) {
    return (int)(sp.total_steps / (int32_t)sp.steps_per_rev);
}

void spindle_reset_turns(SpindleMotor &sp) {
    sp.total_steps = 0;
}

void spindle_poll_update(SpindleMotor &sp) {
    sp.rpm = (float)sp.i_rpm_x100 / 100.0f;
}
