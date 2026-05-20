#pragma once

// === Motion Controller Pin Definitions ===
// Target: ESP32-S3-DevKitC-1 with DM542 stepper drivers
// 2x Rattmotor ZBX80 stages + 1 spindle motor
//
// DM542 wiring per axis:
//   PUL+ → GPIO (step)    PUL- → GND
//   DIR+ → GPIO (dir)     DIR- → GND
//   ENA+ → GPIO (enable)  ENA- → GND
//   (active LOW enable — LOW = motor energized)

// UART to HMI (CrowPanel P4)
// Connect: ESP32-S3 TX → P4 GPIO44 (RX)
//          ESP32-S3 RX → P4 GPIO43 (TX)
#define PIN_HMI_TX       17
#define PIN_HMI_RX       18
#define HMI_BAUD          500000

// X axis — traverse (wire guide left/right)
// Rattmotor ZBX80 200mm, SFU1605 ballscrew
#define PIN_X_STEP         1
#define PIN_X_DIR          2
#define PIN_X_EN           3

// Y axis — radial (wire guide in/out from bobbin)
// Rattmotor ZBX80 200mm, SFU1605 ballscrew
#define PIN_Y_STEP         4
#define PIN_Y_DIR          5
#define PIN_Y_EN           6

// Z axis — not used (set to -1), kept for future expansion
#define PIN_Z_STEP        -1
#define PIN_Z_DIR         -1
#define PIN_Z_EN          -1

// Spindle motor (rotates the bobbin/winding form)
// NEMA23 or larger, with DM542 or dedicated servo driver
#define PIN_SPINDLE_STEP   7
#define PIN_SPINDLE_DIR    8
#define PIN_SPINDLE_EN    15

// Limit switches (active LOW, internal pullup)
#define PIN_X_HOME        10
#define PIN_Y_HOME        11
#define PIN_Z_HOME        -1  // not used

// E-stop input (active LOW, internal pullup)
#define PIN_ESTOP         12
