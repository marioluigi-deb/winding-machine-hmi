#pragma once
#include <lvgl.h>
#include "../config.h"

// Screen IDs
enum class Screen : uint8_t {
    SPLASH,
    HOME,
    SETUP,
    MANUAL,
    RUN,
    SETTINGS,
    WIFI
};

// Colors — original dark navy theme
#define COLOR_BG         lv_color_hex(0x1A1A2E)
#define COLOR_CARD       lv_color_hex(0x16213E)
#define COLOR_PRIMARY    lv_color_hex(0x0F3460)
#define COLOR_ACCENT     lv_color_hex(0x00B4D8)
#define COLOR_GREEN      lv_color_hex(0x06D6A0)
#define COLOR_RED        lv_color_hex(0xE63946)
#define COLOR_ORANGE     lv_color_hex(0xF77F00)
#define COLOR_YELLOW     lv_color_hex(0xFFD60A)
#define COLOR_TEXT       lv_color_hex(0xEAEAEA)
#define COLOR_TEXT_DIM   lv_color_hex(0x8899AA)

// Shared state for UI
extern WindingProgram   ui_program;
extern MachineConfig    ui_config;
extern int              ui_selected_slot;

// Init all screens
void ui_init();
void ui_update();
void ui_navigate(Screen scr);

// Individual screen constructors
lv_obj_t *ui_home_create(lv_obj_t *parent);
lv_obj_t *ui_setup_create(lv_obj_t *parent);
lv_obj_t *ui_manual_create(lv_obj_t *parent);
lv_obj_t *ui_run_create(lv_obj_t *parent);
lv_obj_t *ui_settings_create(lv_obj_t *parent);
lv_obj_t *ui_wifi_create(lv_obj_t *parent);
lv_obj_t *ui_splash_create(lv_obj_t *parent);

// Screen update callbacks (called every frame when screen is active)
void ui_splash_update();
void ui_home_update();
void ui_setup_update();
void ui_manual_update();
void ui_run_update();
void ui_settings_update();
void ui_wifi_update();

// Helpers
lv_obj_t *ui_create_btn(lv_obj_t *parent, const char *text, int w, int h,
                         lv_color_t bg, lv_event_cb_t cb);
lv_obj_t *ui_create_label(lv_obj_t *parent, const char *text,
                           const lv_font_t *font, lv_color_t color);
lv_obj_t *ui_create_card(lv_obj_t *parent, int w, int h);

const char *machine_status_str(MachineStatus st);
lv_color_t machine_status_color(MachineStatus st);
