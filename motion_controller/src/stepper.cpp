#include "stepper.h"
#include <Arduino.h>

void stepper_init(Stepper &s) {
    pinMode(s.pin_step, OUTPUT);
    pinMode(s.pin_dir, OUTPUT);
    pinMode(s.pin_en, OUTPUT);
    if (s.pin_home != 0xFF)
        pinMode(s.pin_home, INPUT_PULLUP);

    digitalWrite(s.pin_en, HIGH);  // disabled by default (active LOW)
    digitalWrite(s.pin_step, LOW);
    s.position = 0;
    s.target = 0;
    s.velocity = 0;
    s.target_vel = 0;
    s.moving = false;
    s.homed = false;
    s.dir_invert = false;
}

void spindle_init(SpindleMotor &sp) {
    pinMode(sp.pin_step, OUTPUT);
    pinMode(sp.pin_dir, OUTPUT);
    pinMode(sp.pin_en, OUTPUT);
    digitalWrite(sp.pin_en, HIGH);
    digitalWrite(sp.pin_step, LOW);
    sp.rpm = 0;
    sp.target_rpm = 0;
    sp.total_steps = 0;
    sp.direction = 0;
    sp.running = false;
}

// Trapezoidal acceleration profile
void stepper_isr_tick(Stepper &s, float dt) {
    if (!s.moving) return;

    int32_t dist = s.target - s.position;
    if (dist == 0) {
        s.moving = false;
        s.velocity = 0;
        return;
    }

    float dir = (dist > 0) ? 1.0f : -1.0f;
    float abs_dist = abs(dist);

    // Decel distance (steps needed to decel to 0 from current velocity)
    float abs_vel = fabsf(s.velocity);
    float decel_steps = (abs_vel * abs_vel) / (2.0f * s.accel * s.steps_per_mm);

    if (abs_dist <= decel_steps + 1) {
        // Decelerating
        abs_vel -= s.accel * s.steps_per_mm * dt;
        if (abs_vel < s.steps_per_mm * 0.5f) abs_vel = s.steps_per_mm * 0.5f;
    } else if (abs_vel < s.target_vel) {
        // Accelerating
        abs_vel += s.accel * s.steps_per_mm * dt;
        if (abs_vel > s.target_vel) abs_vel = s.target_vel;
    }

    s.velocity = abs_vel * dir;

    // Generate step pulse if velocity warrants it
    float steps_this_tick = fabsf(s.velocity) * dt;
    if (steps_this_tick >= 0.5f) {
        bool fwd = (s.velocity > 0) ^ s.dir_invert;
        digitalWrite(s.pin_dir, fwd ? HIGH : LOW);
        digitalWrite(s.pin_step, HIGH);
        delayMicroseconds(2);
        digitalWrite(s.pin_step, LOW);
        s.position += (s.velocity > 0) ? 1 : -1;
    }
}

void spindle_isr_tick(SpindleMotor &sp, float dt) {
    if (!sp.running || sp.direction == 0) return;

    // Ramp RPM toward target
    float diff = sp.target_rpm - sp.rpm;
    float ramp = 200.0f * dt;  // 200 RPM/s ramp rate
    if (fabsf(diff) < ramp) {
        sp.rpm = sp.target_rpm;
    } else {
        sp.rpm += (diff > 0) ? ramp : -ramp;
    }

    if (sp.rpm < 1.0f) {
        sp.running = false;
        return;
    }

    // Step frequency = (RPM / 60) * steps_per_rev
    float step_freq = (sp.rpm / 60.0f) * sp.steps_per_rev;
    float steps_this_tick = step_freq * dt;

    if (steps_this_tick >= 0.5f) {
        digitalWrite(sp.pin_dir, (sp.direction == 1) ? HIGH : LOW);
        digitalWrite(sp.pin_step, HIGH);
        delayMicroseconds(2);
        digitalWrite(sp.pin_step, LOW);
        sp.total_steps++;
    }
}

void stepper_move_to(Stepper &s, float pos_mm, float feed_mms) {
    s.target = (int32_t)(pos_mm * s.steps_per_mm);
    s.target_vel = feed_mms * s.steps_per_mm;
    if (s.target_vel > s.max_feed * s.steps_per_mm)
        s.target_vel = s.max_feed * s.steps_per_mm;
    s.moving = true;
    digitalWrite(s.pin_en, LOW);  // enable
}

void stepper_jog(Stepper &s, float dist_mm) {
    float current_mm = (float)s.position / s.steps_per_mm;
    stepper_move_to(s, current_mm + dist_mm, s.max_feed * 0.5f);
}

void stepper_stop(Stepper &s) {
    s.target = s.position;
    s.moving = false;
    s.velocity = 0;
}

bool stepper_home(Stepper &s, float feed_mms) {
    if (s.pin_home == 0xFF) return false;
    // Move toward home switch
    s.target = s.position - (int32_t)(300.0f * s.steps_per_mm); // 300mm max travel
    s.target_vel = feed_mms * s.steps_per_mm;
    s.moving = true;
    digitalWrite(s.pin_en, LOW);
    return true;
}

float stepper_pos_mm(const Stepper &s) {
    return (float)s.position / s.steps_per_mm;
}

void spindle_set_rpm(SpindleMotor &sp, float rpm) {
    sp.target_rpm = rpm;
    if (rpm > 0 && sp.direction != 0) {
        sp.running = true;
        digitalWrite(sp.pin_en, LOW);
    }
}

void spindle_set_dir(SpindleMotor &sp, int dir) {
    sp.direction = dir;
    if (dir == 0) {
        sp.target_rpm = 0;
    } else {
        sp.running = true;
        digitalWrite(sp.pin_en, LOW);
    }
}

void spindle_stop(SpindleMotor &sp) {
    sp.target_rpm = 0;
    sp.direction = 0;
}

int spindle_turn_count(const SpindleMotor &sp) {
    return (int)(sp.total_steps / sp.steps_per_rev);
}

void spindle_reset_turns(SpindleMotor &sp) {
    sp.total_steps = 0;
}
