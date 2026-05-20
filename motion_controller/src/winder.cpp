#include "winder.h"
#include <Arduino.h>
#include <math.h>

void Winder::init(Stepper *traverse, Stepper *radial, Stepper *vertical,
                  SpindleMotor *spindle) {
    m_traverse = traverse;
    m_radial   = radial;
    m_vertical = vertical;
    m_spindle  = spindle;
    m_status.state = WinderState::IDLE;
}

void Winder::load_program(const WinderParams &params) {
    m_params = params;

    // Auto-calculate traverse pitch = wire OD per turn
    m_traverse_pitch = params.wire_od_mm;

    // Turns per layer = bobbin_width / wire_od
    m_turns_per_layer = (int)(params.bobbin_width_mm / params.wire_od_mm);
    if (m_turns_per_layer < 1) m_turns_per_layer = 1;

    m_status.current_turn = 0;
    m_status.current_layer = 0;
    m_status.state = WinderState::IDLE;
}

void Winder::start() {
    if (m_status.state != WinderState::IDLE) return;

    m_status.state = WinderState::WINDING;
    m_status.current_turn = 0;
    m_status.current_layer = 0;
    m_turns_this_layer = 0;
    m_traverse_forward = true;
    m_last_turn = 0;

    spindle_reset_turns(*m_spindle);
    spindle_set_dir(*m_spindle, m_params.direction_cw ? 1 : 2);
    spindle_set_rpm(*m_spindle, m_params.speed_rpm);

    // Move traverse to start position
    stepper_move_to(*m_traverse, 0.0f, m_traverse->max_feed);
}

void Winder::pause() {
    if (m_status.state == WinderState::WINDING) {
        m_status.state = WinderState::PAUSED;
        spindle_set_rpm(*m_spindle, 0);
    }
}

void Winder::resume() {
    if (m_status.state == WinderState::PAUSED) {
        m_status.state = WinderState::WINDING;
        spindle_set_rpm(*m_spindle, m_params.speed_rpm);
    }
}

void Winder::abort() {
    m_status.state = WinderState::IDLE;
    spindle_stop(*m_spindle);
    stepper_stop(*m_traverse);
    stepper_stop(*m_radial);
}

void Winder::set_speed(float rpm) {
    m_params.speed_rpm = rpm;
    if (m_status.state == WinderState::WINDING) {
        spindle_set_rpm(*m_spindle, rpm);
    }
}

void Winder::update() {
    if (m_status.state != WinderState::WINDING &&
        m_status.state != WinderState::LAYER_REVERSE) return;

    int current_turn = spindle_turn_count(*m_spindle);
    m_status.current_turn = current_turn;

    // Check completion
    if (current_turn >= m_params.total_turns) {
        m_status.state = WinderState::COMPLETE;
        spindle_stop(*m_spindle);
        stepper_stop(*m_traverse);
        return;
    }

    // Track per-turn traverse movement
    if (current_turn != m_last_turn) {
        int new_turns = current_turn - m_last_turn;
        m_last_turn = current_turn;
        m_turns_this_layer += new_turns;

        advance_traverse();

        // Check layer completion
        if (m_turns_this_layer >= m_turns_per_layer) {
            next_layer();
        }
    }

    m_status.traverse_pos = stepper_pos_mm(*m_traverse);
}

void Winder::advance_traverse() {
    // Move traverse by one wire pitch per spindle turn
    float move = m_traverse_forward ? m_traverse_pitch : -m_traverse_pitch;
    float new_pos = stepper_pos_mm(*m_traverse) + move;

    // Clamp to bobbin width
    if (new_pos < 0) new_pos = 0;
    if (new_pos > m_params.bobbin_width_mm) new_pos = m_params.bobbin_width_mm;

    // Traverse speed synchronized to spindle RPM
    // At full speed, traverse must move wire_od per revolution
    float trav_speed = m_traverse_pitch * (m_params.speed_rpm / 60.0f);
    stepper_move_to(*m_traverse, new_pos, trav_speed * 2.0f);
}

void Winder::next_layer() {
    m_status.current_layer++;
    m_turns_this_layer = 0;
    m_traverse_forward = !m_traverse_forward;

    // Step radial axis outward by wire diameter (layer builds up)
    if (m_radial) {
        float radial_step = m_params.wire_od_mm;
        if (m_params.layer_step_mm > 0) radial_step = m_params.layer_step_mm;
        float new_y = stepper_pos_mm(*m_radial) + radial_step;
        stepper_move_to(*m_radial, new_y, m_radial->max_feed * 0.3f);
    }
}
