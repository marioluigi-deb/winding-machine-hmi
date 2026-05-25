#pragma once
#include <stdint.h>

// === Board Selection ===
// Set via build flag in platformio.ini: -DBOARD_CROWPANEL_S3 or -DBOARD_CROWPANEL_P4
// If neither is set, default to S3.
#if !defined(BOARD_CROWPANEL_S3) && !defined(BOARD_CROWPANEL_P4)
#define BOARD_CROWPANEL_S3
#endif

// ============================================================
// CrowPanel Advanced 5" (ESP32-P4) pin definitions
// ============================================================
#ifdef BOARD_CROWPANEL_P4

// RGB LCD data bus (16-bit from P4 repo)
#define PIN_LCD_DE       2
#define PIN_LCD_VSYNC   41
#define PIN_LCD_HSYNC   40
#define PIN_LCD_PCLK     3
#define PIN_LCD_R0       4
#define PIN_LCD_R1       5
#define PIN_LCD_R2       6
#define PIN_LCD_R3       7
#define PIN_LCD_R4       8
#define PIN_LCD_G0       9
#define PIN_LCD_G1      10
#define PIN_LCD_G2      11
#define PIN_LCD_G3      12
#define PIN_LCD_G4      13
#define PIN_LCD_G5      14
#define PIN_LCD_B0      15
#define PIN_LCD_B1      16
#define PIN_LCD_B2      17
#define PIN_LCD_B3      18
#define PIN_LCD_B4      19
// Backlight: STC8 I2C (0x2F), not GPIO — no PIN_LCD_BL needed on P4

// Touch (GT911 I2C) — P4 pinout
#define PIN_TOUCH_SDA   45
#define PIN_TOUCH_SCL   46
#define PIN_TOUCH_INT   42
#define PIN_TOUCH_RST   36
#define TOUCH_I2C_ADDR  0x5D

// === Stepper driver pins (direct step/dir to DM542 drivers) ===
// CrowPanel P4 expansion header GPIOs: 26, 29, 30, 31, 32, 47, 48
#define PIN_X_STEP      47
#define PIN_X_DIR       48
#define PIN_Y_STEP      29
#define PIN_Y_DIR       30
#define PIN_SP_STEP     31
#define PIN_SP_DIR      32
#define PIN_ESTOP       26    // E-stop input, active LOW with internal pullup

#endif // BOARD_CROWPANEL_P4

// ============================================================
// CrowPanel Advance 5.0" (ESP32-S3) pin definitions
// ============================================================
#ifdef BOARD_CROWPANEL_S3

#define PIN_LCD_DE      40
#define PIN_LCD_VSYNC   41
#define PIN_LCD_HSYNC   39
#define PIN_LCD_PCLK    42
#define PIN_LCD_R0      45
#define PIN_LCD_R1      48
#define PIN_LCD_R2      47
#define PIN_LCD_R3      21
#define PIN_LCD_R4      14
#define PIN_LCD_G0       5
#define PIN_LCD_G1       6
#define PIN_LCD_G2       7
#define PIN_LCD_G3      15
#define PIN_LCD_G4      16
#define PIN_LCD_G5       4
#define PIN_LCD_B0       8
#define PIN_LCD_B1       3
#define PIN_LCD_B2      46
#define PIN_LCD_B3       9
#define PIN_LCD_B4       1
#define PIN_LCD_BL       2

#define PIN_TOUCH_SDA   19
#define PIN_TOUCH_SCL   20
#define PIN_TOUCH_INT   18
#define PIN_TOUCH_RST   38
#define TOUCH_I2C_ADDR  0x5D

#define PIN_MC_TX       43
#define PIN_MC_RX       44
#define MC_BAUD         115200

#endif // BOARD_CROWPANEL_S3

// Display
#define SCREEN_W        800
#define SCREEN_H        480

// === Machine Defaults ===
// Rattmotor ZBX80: SFU1605 ballscrew (5mm lead), NEMA23 200step, 16x microstep
// 200 * 16 / 5 = 640 steps/mm
#define DEFAULT_STEPS_PER_MM_X   640.0f
#define DEFAULT_STEPS_PER_MM_Y   640.0f
#define DEFAULT_STEPS_PER_MM_Z   640.0f
#define DEFAULT_STEPS_PER_REV    3200.0f   // NEMA23 200step * 16x
#define DEFAULT_MAX_FEED_X       1200.0f   // mm/min (20 mm/s)
#define DEFAULT_MAX_FEED_Y       1200.0f
#define DEFAULT_MAX_FEED_Z       1200.0f
#define DEFAULT_MAX_SPINDLE_RPM  600.0f
#define DEFAULT_ACCEL_X          200.0f    // mm/s^2
#define DEFAULT_ACCEL_Y          200.0f
#define DEFAULT_ACCEL_Z          200.0f

// === Winding Program ===
struct WindingProgram {
    char     name[32];
    float    wire_od_mm;
    int      awg;
    float    bobbin_width_mm;
    float    bobbin_id_mm;
    int      total_turns;
    int      num_layers;
    float    speed_rpm;
    float    traverse_start_mm;
    bool     direction_cw;
    float    layer_step_mm;
};

// === Machine State ===
enum class MachineStatus : uint8_t {
    DISCONNECTED,
    IDLE,
    HOMING,
    MOVING,
    WINDING,
    PAUSED,
    ESTOP,
    ERROR
};

struct MachineState {
    MachineStatus status;
    float pos_x;
    float pos_y;
    float pos_z;
    float spindle_rpm;
    int   current_turn;
    int   current_layer;
    bool  homed_x;
    bool  homed_y;
    bool  homed_z;
    char  error_msg[64];
};

// === WiFi Config ===
struct WiFiConfig {
    char     ssid[33];
    char     password[65];
    char     hostname[32];
    char     notify_url[128];
    bool     auto_connect;
    bool     notify_on_complete;
    bool     notify_on_error;
};

// === Machine Config (saved to NVS) ===
struct MachineConfig {
    float steps_per_mm_x;
    float steps_per_mm_y;
    float steps_per_mm_z;
    float steps_per_rev;
    float max_feed_x;
    float max_feed_y;
    float max_feed_z;
    float max_spindle_rpm;
    float accel_x;
    float accel_y;
    float accel_z;
};
