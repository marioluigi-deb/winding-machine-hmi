#pragma once
#include "stepper.h"

enum class WinderState : uint8_t {
    IDLE,
    WINDING,
    PAUSED,
    LAYER_REVERSE,
    COMPLETE,
    ERROR
};

struct WinderParams {
    int   total_turns;
    int   num_layers;
    float bobbin_width_mm;
    float bobbin_id_mm;
    float wire_od_mm;
    float speed_rpm;
    bool  direction_cw;
    float layer_step_mm;
};

struct WinderStatus {
    WinderState state;
    int   current_turn;
    int   current_layer;
    float traverse_pos;
};

class Winder {
public:
    void init(Stepper *traverse, Stepper *radial, Stepper *vertical_or_null,
              SpindleMotor *spindle);

    void load_program(const WinderParams &params);
    void start();
    void pause();
    void resume();
    void abort();
    void set_speed(float rpm);

    // Call this from the main loop — coordinates spindle + traverse
    void update();

    const WinderStatus &status() const { return m_status; }

private:
    Stepper      *m_traverse = nullptr;   // X
    Stepper      *m_radial   = nullptr;   // Y
    Stepper      *m_vertical = nullptr;   // Z
    SpindleMotor *m_spindle  = nullptr;

    WinderParams  m_params = {};
    WinderStatus  m_status = {};

    int   m_turns_this_layer = 0;
    int   m_turns_per_layer = 0;
    bool  m_traverse_forward = true;
    float m_traverse_pitch = 0;
    int   m_last_turn = 0;

    void advance_traverse();
    void next_layer();
};
