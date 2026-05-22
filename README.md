# Winding Machine HMI

A complete motor winding machine controller with touchscreen HMI, web dashboard, WiFi remote monitoring, and integrated stepper motor control — all running on a single **CrowPanel Advanced 5" ESP32-P4** display board.

![Platform](https://img.shields.io/badge/Platform-ESP32--P4-blue)
![Display](https://img.shields.io/badge/Display-800x480_IPS-green)
![Framework](https://img.shields.io/badge/Framework-Arduino-teal)

## Features

- **5" Touchscreen HMI** — 6 screens: Home, Program Setup, Manual Jog, Active Winding, Settings, WiFi
- **Dark industrial theme** — Navy/teal color scheme, large touch targets for workshop use
- **Integrated motion control** — 20kHz step generation ISR (integer-only for P4 RISC-V compatibility)
- **Web dashboard** — Live-updating status via Server-Sent Events, accessible from phone/laptop
- **OTA firmware updates** — Upload new firmware from a browser
- **REST API** — Full program management, machine control, and status endpoints
- **Webhook notifications** — HTTP POST on winding complete, errors, or E-stop
- **WiFi via ESP32-C6** — On-board co-processor provides WiFi 6 + BLE 5.3

## Hardware

### Bill of Materials

| Item | Qty | Specs | Approx Cost |
|------|-----|-------|-------------|
| CrowPanel Advanced 5" ESP32-P4 | 1 | 800x480 IPS, ESP32-P4 + ESP32-C6, 32MB PSRAM, 16MB Flash | $45 |
| Rattmotor ZBX80 200mm Linear Stage | 2 | SFU1605 ballscrew (5mm lead), NEMA23 stepper, 200mm stroke | $80 ea |
| DM542 Stepper Driver | 2 | 4.2A peak, step/dir interface, 24-48V input | $15 ea |
| 24V Power Supply | 1 | 24V 10A (240W) for stepper drivers | $20 |
| USB-C cable | 1 | For programming the CrowPanel | $5 |
| Hookup wire | — | 22AWG for signal, 18AWG for motor power | $10 |

**Total: ~$260**

### Wiring Diagram

The CrowPanel P4 exposes GPIOs through **Crowtail 4-pin connectors**. Each connector has VCC, GND, and 2 signal pins.

```
CrowPanel Crowtail Connectors         DM542 Stepper Drivers
═══════════════════════════════         ══════════════════════

UART1 Connector                        DM542 #1 (X Traverse)
┌──────────────────────┐               ┌──────────────────────┐
│ VCC  GND  47    48   │               │                      │
│           TX    RX   │               │  PUL+  DIR+  ENA+    │
└──────┬───┬──────┬────┘               │  PUL-  DIR-  ENA-    │
       │   │      │                    └──┬────┬────┬─────────┘
       │   │      │                       │    │    │
       │   │      └── wire ──────────────►│DIR+│    │
       │   └── wire ──► PUL- DIR- ENA-────┘    │    │
       └────── wire ────────────────────►PUL+   │    │
                                                │    │
               GND ──── wire ──────────────────►ENA+ │
               GND ──── wire ──────────────────►ENA- ┘

UART0 Connector                        DM542 #2 (Y Radial)
┌──────────────────────┐               ┌──────────────────────┐
│ VCC  GND  43    44   │               │                      │
│           TX    RX   │               │  PUL+  DIR+  ENA+    │
└──────┬───┬──────┬────┘               │  PUL-  DIR-  ENA-    │
       │   │      │                    └──┬────┬────┬─────────┘
       │   │      │                       │    │    │
       │   │      └── wire ──────────────►│DIR+│    │
       │   └── wire ──► PUL- DIR- ENA-────┘    │    │
       └────── wire ────────────────────►PUL+   │    │
                                                │    │
               GND ──── wire ──────────────────►ENA+ │
               GND ──── wire ──────────────────►ENA- ┘
```

**Simplified wiring per DM542:**

| CrowPanel Pin | DM542 Pin | Wire Color Suggestion |
|---------------|-----------|----------------------|
| GPIO 47 (UART1 TX) | DM542 #1 PUL+ | Yellow |
| GPIO 48 (UART1 RX) | DM542 #1 DIR+ | Green |
| GND (UART1) | DM542 #1 PUL-, DIR-, ENA+, ENA- | Black (4 wires or daisy-chain) |
| GPIO 43 (UART0 TX) | DM542 #2 PUL+ | Yellow |
| GPIO 44 (UART0 RX) | DM542 #2 DIR+ | Green |
| GND (UART0) | DM542 #2 PUL-, DIR-, ENA+, ENA- | Black |

**Power (separate from signal):**

```
24V Power Supply
────────────────
24V+ ───────► DM542 #1 V+  AND  DM542 #2 V+      (red wire, 18AWG)
24V- (GND) ─► DM542 #1 V-  AND  DM542 #2 V-      (black wire, 18AWG)

USB-C ──────► CrowPanel (5V power + programming)
```

**Motor wires (NEMA23 to DM542):**

```
Rattmotor ZBX80 #1 motor cable ───► DM542 #1: A+ A- B+ B-
Rattmotor ZBX80 #2 motor cable ───► DM542 #2: A+ A- B+ B-
```

**DM542 DIP Switch Settings (16x microstepping, 3A peak):**

| Switch | Position | Function |
|--------|----------|----------|
| SW1 | ON | Peak current 3A |
| SW2 | ON | |
| SW3 | OFF | |
| SW4 | OFF | Pulse active rising edge |
| SW5 | ON | 16x microstepping |
| SW6 | ON | |
| SW7 | OFF | |
| SW8 | OFF | |

> **Note:** The DM542 ENA+ and ENA- are both tied to GND, keeping the driver always enabled. This is because the CrowPanel only has 4 accessible GPIOs on the Crowtail connectors (2 per connector), and all 4 are used for step + direction signals.

### Machine Axes

| Axis | Function | Hardware | Connector | GPIOs | Travel |
|------|----------|----------|-----------|-------|--------|
| X | Traverse — guides wire left/right across bobbin | Rattmotor ZBX80 + DM542 | UART1 Crowtail | 47 (step), 48 (dir) | 200mm |
| Y | Radial — adjusts wire guide distance as layers build | Rattmotor ZBX80 + DM542 | UART0 Crowtail | 43 (step), 44 (dir) | 200mm |
| Spindle | Rotates the winding bobbin/form | Not connected (no free GPIOs) | — | — | — |

### Motion Parameters (defaults, adjustable in Settings screen)

| Parameter | Value | Notes |
|-----------|-------|-------|
| Steps/mm (X, Y) | 640 | SFU1605: 200 steps x 16 microstep / 5mm lead |
| Steps/rev (spindle) | 3200 | 200 steps x 16 microstep |
| Max feed (X, Y) | 1200 mm/min | 20 mm/s |
| Acceleration | 200 mm/s^2 | |
| Max spindle RPM | 600 | Adjustable per program |

## Getting Started

### Prerequisites

- [PlatformIO CLI](https://docs.platformio.org/en/latest/core/installation.html) or PlatformIO IDE
- USB-C cable
- CrowPanel Advanced 5" ESP32-P4 board

### 1. Clone and Build

```bash
git clone https://github.com/YOUR_USERNAME/winding-machine-hmi.git
cd winding-machine-hmi
pio run -e crowpanel_p4
```

First build takes ~10 minutes (downloads toolchain + ESP-IDF 5.5). Subsequent builds take ~35 seconds.

### 2. Configure WiFi

Edit `src/wifi_manager.cpp` and set your WiFi credentials in the default config block:

```cpp
strlcpy(cfg.ssid, "YOUR_SSID", sizeof(cfg.ssid));
strlcpy(cfg.password, "YOUR_PASSWORD", sizeof(cfg.password));
```

Or configure later via the touchscreen WiFi settings page (Settings > WiFi).

### 3. Flash

Connect the CrowPanel via USB-C and flash:

```bash
pio run -e crowpanel_p4 -t upload
```

### 4. First Boot

The display will show the winding machine dashboard with:
- Machine status (IDLE)
- Position readouts (X, Y)
- Navigation buttons: SETUP, MANUAL, RUN, CONFIG

### 5. Web Dashboard

Connect to the same WiFi network and open `http://<board-ip>` in a browser. The IP address is shown on the serial monitor during boot. The dashboard provides:
- Live status updates (5Hz via SSE)
- Start/Pause/Resume/Abort controls
- Turn counter and progress bar

### 6. OTA Updates

After the first flash via USB, subsequent updates can be done over WiFi:
1. Build: `pio run -e crowpanel_p4`
2. Open `http://<board-ip>/update`
3. Login: admin / winder
4. Upload `.pio/build/crowpanel_p4/firmware.bin`

### 7. Create a Winding Program

On the touchscreen, tap **SETUP**:
1. Enter program name
2. Select wire gauge (AWG 10-40, auto-calculates wire OD)
3. Set total turns, layers, bobbin width/diameter
4. Set winding speed (RPM) and direction
5. Save to one of 8 program slots
6. Tap **START WINDING**

Programs can also be uploaded via the REST API:
```bash
curl -X POST http://<board-ip>/api/program?slot=0 \
  -H "Content-Type: application/json" \
  -d '{"name":"Test","awg":24,"turns":200,"layers":2,"bobbin_width":50,"bobbin_id":25,"speed":200,"direction_cw":true}'
```

## API Reference

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/status` | Machine state JSON |
| GET | `/api/events` | SSE live stream (5Hz) |
| GET | `/api/programs` | List all programs |
| GET | `/api/program?slot=N` | Get program details |
| POST | `/api/program?slot=N` | Save program (JSON body) |
| DELETE | `/api/program?slot=N` | Delete program |
| POST | `/api/control/start` | Start winding |
| POST | `/api/control/pause` | Pause |
| POST | `/api/control/resume` | Resume |
| POST | `/api/control/abort` | Abort |
| POST | `/api/control/estop` | Emergency stop |
| POST | `/api/control/reset` | Reset after E-stop |
| POST | `/api/control/speed?rpm=N` | Change speed |
| GET | `/api/wifi` | WiFi config |
| POST | `/api/wifi` | Save WiFi config (JSON) |

## Project Structure

```
winding_machine_hmi/
├── platformio.ini              # PlatformIO build config
├── lv_conf.h                   # LVGL 8.3 configuration
├── src/
│   ├── main.cpp                # Setup + loop
│   ├── config.h                # Pin defs, structs, machine defaults
│   ├── display.h/cpp           # RGB LCD + GT911 touch + LVGL init
│   ├── motion.h/cpp            # Stepper control + winding state machine
│   ├── stepper.h/cpp           # Step generation (integer-only ISR)
│   ├── winder.h/cpp            # Winding program logic
│   ├── storage.h/cpp           # NVS save/load (programs, config, WiFi)
│   ├── wifi_manager.h/cpp      # WiFi STA/AP, mDNS, ESP-Hosted C6
│   ├── web_server.h/cpp        # HTTP server, REST API, SSE, OTA
│   ├── notifications.h/cpp     # Webhook alerts
│   ├── sound.h/cpp             # Audio (stub, I2S deferred)
│   ├── board_config.h          # CrowPanel P4 pin mapping (from Elecrow)
│   └── ui/
│       ├── ui.h/cpp            # Screen manager, theme, helpers
│       ├── ui_home.cpp         # Dashboard screen
│       ├── ui_setup.cpp        # Program editor
│       ├── ui_manual.cpp       # Manual jog + spindle control
│       ├── ui_run.cpp          # Active winding screen
│       ├── ui_settings.cpp     # Machine config
│       └── ui_wifi.cpp         # WiFi settings
└── motion_controller/          # Standalone firmware (if using external ESP32)
    ├── platformio.ini
    └── src/
        ├── main.cpp
        ├── pins.h
        ├── stepper.h/cpp
        └── winder.h/cpp
```

## Platform Notes (ESP32-P4 + CrowPanel)

Hard-won lessons from development:

- **ESP-IDF 5.5+ required** — P4 RGB LCD driver (`esp_lcd_new_rgb_panel`) wasn't available until ESP-IDF 5.5
- **`flags.fb_in_psram = 1`** — 800x480 framebuffer (768KB) must be in PSRAM
- **WiFi before display** — ESP-Hosted SDIO init changes PSRAM bus config; LCD must init after
- **Backlight last** — Turn on STC8 backlight after WiFi + LVGL redraw to avoid seeing corruption
- **No float in ISR** — P4 RISC-V forbids FPU in interrupts; step generation is pure integer
- **No `WiFi.mode(WIFI_STA)`** — Blocks ESP-Hosted events; just call `WiFi.begin()` directly
- **STC8H1KXX backlight** — Controlled via I2C (addr 0x2F), not GPIO
- **Wire only** — Old ESP-IDF I2C driver conflicts with Arduino Wire; use Wire exclusively
- **GPIO access limited** — Only 4 GPIOs accessible via Crowtail connectors (43, 44, 47, 48). GPIOs 33-39 cause hangs (flash/PSRAM conflict). Internal GPIOs 0-19, 20-23 are used by LCD, audio, and SDIO.
- **I2S + LCD DMA conflict** — I2S audio crashes when LCD RGB DMA is active (P4 GDMA conflict in ESP-IDF 5.5). Onboard speaker unusable. Use external piezo buzzer on a Crowtail GPIO if sound is needed.
- **Stepper enable** — DM542 ENA+/ENA- both tied to GND (always enabled) since no free GPIOs for enable control.

## License

MIT
