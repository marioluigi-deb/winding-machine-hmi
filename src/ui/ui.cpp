#include "ui.h"
#include "../motion.h"
#include "../storage.h"

WindingProgram ui_program = {};
MachineConfig  ui_config  = {};
int            ui_selected_slot = 0;

static lv_obj_t *screens[6] = {};
static Screen    active_screen = Screen::HOME;

const char *machine_status_str(MachineStatus st) {
    switch (st) {
        case MachineStatus::DISCONNECTED: return "DISCONNECTED";
        case MachineStatus::IDLE:         return "IDLE";
        case MachineStatus::HOMING:       return "HOMING";
        case MachineStatus::MOVING:       return "MOVING";
        case MachineStatus::WINDING:      return "WINDING";
        case MachineStatus::PAUSED:       return "PAUSED";
        case MachineStatus::ESTOP:        return "E-STOP";
        case MachineStatus::ERROR:        return "ERROR";
    }
    return "?";
}

lv_color_t machine_status_color(MachineStatus st) {
    switch (st) {
        case MachineStatus::DISCONNECTED: return COLOR_TEXT_DIM;
        case MachineStatus::IDLE:         return COLOR_GREEN;
        case MachineStatus::HOMING:       return COLOR_ACCENT;
        case MachineStatus::MOVING:       return COLOR_ACCENT;
        case MachineStatus::WINDING:      return COLOR_GREEN;
        case MachineStatus::PAUSED:       return COLOR_ORANGE;
        case MachineStatus::ESTOP:        return COLOR_RED;
        case MachineStatus::ERROR:        return COLOR_RED;
    }
    return COLOR_TEXT;
}

lv_obj_t *ui_create_btn(lv_obj_t *parent, const char *text, int w, int h,
                         lv_color_t bg, lv_event_cb_t cb) {
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_style_bg_color(btn, bg, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn, 8, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    if (cb) lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
    lv_obj_center(lbl);
    return btn;
}

lv_obj_t *ui_create_label(lv_obj_t *parent, const char *text,
                           const lv_font_t *font, lv_color_t color) {
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, color, 0);
    lv_obj_set_style_text_font(lbl, font, 0);
    return lbl;
}

lv_obj_t *ui_create_card(lv_obj_t *parent, int w, int h) {
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, w, h);
    lv_obj_set_style_bg_color(card, COLOR_CARD, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_radius(card, 10, 0);
    lv_obj_set_style_pad_all(card, 12, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    return card;
}

void ui_init() {
    // Load saved config and default program
    storage_load_config(ui_config);
    if (!storage_load_program(0, ui_program)) {
        strncpy(ui_program.name, "Default", sizeof(ui_program.name));
        ui_program.wire_od_mm      = 0.52f;
        ui_program.awg             = 24;
        ui_program.bobbin_width_mm = 50.0f;
        ui_program.bobbin_id_mm    = 25.0f;
        ui_program.total_turns     = 200;
        ui_program.num_layers      = 2;
        ui_program.speed_rpm       = 200.0f;
        ui_program.traverse_start_mm = 0.0f;
        ui_program.direction_cw    = true;
        ui_program.layer_step_mm   = 0.0f;
    }

    // Set root background
    lv_obj_set_style_bg_color(lv_scr_act(), COLOR_BG, 0);

    screens[0] = ui_home_create(nullptr);
    screens[1] = ui_setup_create(nullptr);
    screens[2] = ui_manual_create(nullptr);
    screens[3] = ui_run_create(nullptr);
    screens[4] = ui_settings_create(nullptr);
    screens[5] = ui_wifi_create(nullptr);

    ui_navigate(Screen::HOME);
}

void ui_navigate(Screen scr) {
    active_screen = scr;
    lv_scr_load_anim(screens[(int)scr], LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, false);
}

void ui_update() {
    switch (active_screen) {
        case Screen::HOME:     ui_home_update();     break;
        case Screen::SETUP:    ui_setup_update();    break;
        case Screen::MANUAL:   ui_manual_update();   break;
        case Screen::RUN:      ui_run_update();      break;
        case Screen::SETTINGS: ui_settings_update(); break;
        case Screen::WIFI:     ui_wifi_update();     break;
    }
}
