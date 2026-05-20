#include <Arduino.h>
#include "pins.h"
#include "stepper.h"
#include "winder.h"

// ============================================================
// Hardware
// ============================================================
static HardwareSerial HmiSerial(1);

static Stepper axis_x, axis_y;
static SpindleMotor spindle;
static Winder winder;

static bool estop_active = false;
static char rx_buf[256];
static int  rx_pos = 0;
static unsigned long last_status_ms = 0;

// ISR timer for step generation
static hw_timer_t *step_timer = nullptr;
static const uint32_t STEP_ISR_HZ = 20000;
static const float STEP_DT = 1.0f / STEP_ISR_HZ;

void IRAM_ATTR step_timer_isr() {
    if (estop_active) return;
    stepper_isr_tick(axis_x, STEP_DT);
    stepper_isr_tick(axis_y, STEP_DT);
    spindle_isr_tick(spindle, STEP_DT);
}

// ============================================================
// Command parser
// ============================================================

static void send_resp(const char *fmt, ...) {
    char buf[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    HmiSerial.println(buf);
}

static void send_status() {
    const char *st_str = "IDLE";
    auto ws = winder.status().state;
    if (estop_active)                        st_str = "ESTOP";
    else if (ws == WinderState::WINDING)      st_str = "WINDING";
    else if (ws == WinderState::PAUSED)       st_str = "PAUSED";
    else if (ws == WinderState::ERROR)        st_str = "ERROR";
    else if (ws == WinderState::COMPLETE)     st_str = "IDLE";
    else if (axis_x.moving || axis_y.moving) st_str = "MOVING";

    send_resp("PX:%.3f PY:%.3f PZ:0.000 SR:%.0f ST:%s TN:%d LY:%d HX:%d HY:%d HZ:1",
              stepper_pos_mm(axis_x),
              stepper_pos_mm(axis_y),
              spindle.rpm,
              st_str,
              winder.status().current_turn,
              winder.status().current_layer,
              axis_x.homed ? 1 : 0,
              axis_y.homed ? 1 : 0);
}

static float parse_float(const char *p) {
    while (*p && !(*p == '-' || *p == '.' || (*p >= '0' && *p <= '9'))) p++;
    return atof(p);
}

static void process_command(const char *cmd) {
    if (estop_active && strcmp(cmd, "RS") != 0) {
        send_resp("ER E-STOP active, send RS to reset");
        return;
    }

    if (strcmp(cmd, "ST") == 0) {
        send_status();
        return;
    }

    if (strcmp(cmd, "HA") == 0) {
        stepper_home(axis_x, 5.0f);
        stepper_home(axis_y, 5.0f);
        send_resp("OK");
        return;
    }

    if (cmd[0] == 'H' && strlen(cmd) == 2) {
        switch (cmd[1]) {
            case 'X': stepper_home(axis_x, 5.0f); break;
            case 'Y': stepper_home(axis_y, 5.0f); break;
            default: send_resp("ER bad axis"); return;
        }
        send_resp("OK");
        return;
    }

    // MV X10.0 Y20.0 F1000
    if (strncmp(cmd, "MV ", 3) == 0) {
        float x = stepper_pos_mm(axis_x);
        float y = stepper_pos_mm(axis_y);
        float f = 500.0f;
        const char *p = cmd + 3;
        while (*p) {
            switch (*p) {
                case 'X': x = parse_float(p + 1); break;
                case 'Y': y = parse_float(p + 1); break;
                case 'F': f = parse_float(p + 1); break;
            }
            p++;
            while (*p && *p != ' ') p++;
        }
        stepper_move_to(axis_x, x, f / 60.0f);
        stepper_move_to(axis_y, y, f / 60.0f);
        send_resp("OK");
        return;
    }

    if (strncmp(cmd, "JG ", 3) == 0) {
        char ax = cmd[3];
        float dist = parse_float(cmd + 4);
        switch (ax) {
            case 'X': stepper_jog(axis_x, dist); break;
            case 'Y': stepper_jog(axis_y, dist); break;
            default: send_resp("ER bad axis"); return;
        }
        send_resp("OK");
        return;
    }

    if (strncmp(cmd, "SR ", 3) == 0) {
        float rpm = atof(cmd + 3);
        spindle_set_rpm(spindle, rpm);
        if (winder.status().state == WinderState::WINDING)
            winder.set_speed(rpm);
        send_resp("OK");
        return;
    }

    if (strncmp(cmd, "SD ", 3) == 0) {
        int dir = atoi(cmd + 3);
        spindle_set_dir(spindle, dir);
        send_resp("OK");
        return;
    }

    // WL T200 L2 W50.000 D25.000 R0.5200 S200 C1 LS0.520
    if (strncmp(cmd, "WL ", 3) == 0) {
        WinderParams wp = {};
        const char *p = cmd + 3;
        while (*p) {
            if (*p == 'T') wp.total_turns = atoi(p + 1);
            else if (*p == 'L' && *(p+1) == 'S') wp.layer_step_mm = parse_float(p + 2);
            else if (*p == 'L') wp.num_layers = atoi(p + 1);
            else if (*p == 'W') wp.bobbin_width_mm = parse_float(p + 1);
            else if (*p == 'D') wp.bobbin_id_mm = parse_float(p + 1);
            else if (*p == 'R') wp.wire_od_mm = parse_float(p + 1);
            else if (*p == 'S') wp.speed_rpm = parse_float(p + 1);
            else if (*p == 'C') wp.direction_cw = (atoi(p + 1) == 1);
            p++;
            while (*p && *p != ' ') p++;
        }
        winder.load_program(wp);
        send_resp("OK");
        return;
    }

    if (strcmp(cmd, "WS") == 0) { winder.start(); send_resp("OK"); return; }
    if (strcmp(cmd, "WP") == 0) { winder.pause(); send_resp("OK"); return; }
    if (strcmp(cmd, "WR") == 0) { winder.resume(); send_resp("OK"); return; }
    if (strcmp(cmd, "WA") == 0) { winder.abort(); send_resp("OK"); return; }

    if (strcmp(cmd, "ES") == 0) {
        estop_active = true;
        spindle_stop(spindle);
        stepper_stop(axis_x);
        stepper_stop(axis_y);
        winder.abort();
        digitalWrite(PIN_SPINDLE_EN, HIGH);
        digitalWrite(PIN_X_EN, HIGH);
        digitalWrite(PIN_Y_EN, HIGH);
        send_resp("OK");
        return;
    }

    if (strcmp(cmd, "RS") == 0) {
        estop_active = false;
        send_resp("OK");
        return;
    }

    if (strncmp(cmd, "CF ", 3) == 0) {
        char key[16];
        float val;
        if (sscanf(cmd + 3, "%15[^:]:%f", key, &val) == 2) {
            if (strcmp(key, "SPM_X") == 0) axis_x.steps_per_mm = val;
            else if (strcmp(key, "SPM_Y") == 0) axis_y.steps_per_mm = val;
            else if (strcmp(key, "SPR") == 0)   spindle.steps_per_rev = val;
            else if (strcmp(key, "MF_X") == 0)  axis_x.max_feed = val;
            else if (strcmp(key, "MF_Y") == 0)  axis_y.max_feed = val;
            else if (strcmp(key, "ACC_X") == 0) axis_x.accel = val;
            else if (strcmp(key, "ACC_Y") == 0) axis_y.accel = val;
            send_resp("OK");
        } else {
            send_resp("ER bad config format");
        }
        return;
    }

    send_resp("ER unknown command");
}

// ============================================================
// Limit switches and e-stop
// ============================================================

static void check_limits() {
    if (digitalRead(PIN_ESTOP) == LOW && !estop_active) {
        estop_active = true;
        spindle_stop(spindle);
        stepper_stop(axis_x);
        stepper_stop(axis_y);
        winder.abort();
        send_resp("ER E-STOP triggered");
    }

    if (axis_x.moving && PIN_X_HOME >= 0 && digitalRead(PIN_X_HOME) == LOW) {
        stepper_stop(axis_x);
        axis_x.position = 0;
        axis_x.homed = true;
    }
    if (axis_y.moving && PIN_Y_HOME >= 0 && digitalRead(PIN_Y_HOME) == LOW) {
        stepper_stop(axis_y);
        axis_y.position = 0;
        axis_y.homed = true;
    }
}

// ============================================================
// Setup
// ============================================================

void setup() {
    Serial.begin(115200);
    HmiSerial.begin(HMI_BAUD, SERIAL_8N1, PIN_HMI_RX, PIN_HMI_TX);
    Serial.println("Winding Machine Motion Controller");
    Serial.println("  2x Rattmotor ZBX80 (SFU1605, 640 steps/mm @ 16x)");
    Serial.println("  Spindle: NEMA23 (3200 steps/rev @ 16x)");

    pinMode(PIN_ESTOP, INPUT_PULLUP);

    // X axis — traverse
    // SFU1605: 5mm lead, NEMA23 200 steps/rev, 16x microstepping
    // 200 * 16 / 5 = 640 steps/mm
    axis_x = {};
    axis_x.pin_step = PIN_X_STEP;
    axis_x.pin_dir = PIN_X_DIR;
    axis_x.pin_en = PIN_X_EN;
    axis_x.pin_home = PIN_X_HOME;
    axis_x.steps_per_mm = 640.0f;
    axis_x.max_feed = 20.0f;      // 20 mm/s = 1200 mm/min
    axis_x.accel = 200.0f;        // mm/s^2
    stepper_init(axis_x);

    // Y axis — radial
    axis_y = {};
    axis_y.pin_step = PIN_Y_STEP;
    axis_y.pin_dir = PIN_Y_DIR;
    axis_y.pin_en = PIN_Y_EN;
    axis_y.pin_home = PIN_Y_HOME;
    axis_y.steps_per_mm = 640.0f;
    axis_y.max_feed = 20.0f;
    axis_y.accel = 200.0f;
    stepper_init(axis_y);

    // Spindle — NEMA23 200 steps/rev × 16 microsteps = 3200 steps/rev
    spindle = {};
    spindle.pin_step = PIN_SPINDLE_STEP;
    spindle.pin_dir = PIN_SPINDLE_DIR;
    spindle.pin_en = PIN_SPINDLE_EN;
    spindle.steps_per_rev = 3200.0f;
    spindle_init(spindle);

    winder.init(&axis_x, &axis_y, nullptr, &spindle);

    // 20kHz step timer (Arduino ESP32 core 2.x API)
    step_timer = timerBegin(0, 80, true);  // timer 0, prescaler 80 → 1MHz tick
    timerAttachInterrupt(step_timer, &step_timer_isr, true);
    timerAlarmWrite(step_timer, 1000000 / STEP_ISR_HZ, true);  // 50us period
    timerAlarmEnable(step_timer);

    Serial.println("Ready. Listening on UART and USB serial.");
}

void loop() {
    // HMI UART commands
    while (HmiSerial.available()) {
        char c = HmiSerial.read();
        if (c == '\n' || c == '\r') {
            if (rx_pos > 0) {
                rx_buf[rx_pos] = '\0';
                process_command(rx_buf);
                rx_pos = 0;
            }
        } else if (rx_pos < (int)sizeof(rx_buf) - 1) {
            rx_buf[rx_pos++] = c;
        }
    }

    // USB serial debug commands (same protocol)
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            if (rx_pos > 0) {
                rx_buf[rx_pos] = '\0';
                process_command(rx_buf);
                rx_pos = 0;
            }
        } else if (rx_pos < (int)sizeof(rx_buf) - 1) {
            rx_buf[rx_pos++] = c;
        }
    }

    check_limits();
    winder.update();

    if (millis() - last_status_ms > 100) {
        last_status_ms = millis();
        send_status();
    }
}
