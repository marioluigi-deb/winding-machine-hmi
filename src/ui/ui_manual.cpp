#include "ui.h"
#include "../motion.h"
#include <stdio.h>

static lv_obj_t *lbl_x_pos, *lbl_y_pos, *lbl_z_pos;
static lv_obj_t *lbl_spindle_rpm;
static lv_obj_t *slider_spindle;
static float jog_step = 1.0f;
static lv_obj_t *btns_step[4];

static void cb_back(lv_event_t *e) { ui_navigate(Screen::HOME); }

static void highlight_step_btn(int idx) {
    for (int i = 0; i < 4; i++) {
        lv_obj_set_style_bg_color(btns_step[i],
            (i == idx) ? COLOR_ACCENT : COLOR_PRIMARY, 0);
    }
}

static void cb_step_01(lv_event_t *e)  { jog_step = 0.1f;  highlight_step_btn(0); }
static void cb_step_1(lv_event_t *e)   { jog_step = 1.0f;  highlight_step_btn(1); }
static void cb_step_10(lv_event_t *e)  { jog_step = 10.0f; highlight_step_btn(2); }
static void cb_step_100(lv_event_t *e) { jog_step = 100.0f;highlight_step_btn(3); }

static void cb_jog(lv_event_t *e) {
    intptr_t code = (intptr_t)lv_event_get_user_data(e);
    char axis = (char)((code >> 8) & 0xFF);
    int sign = (code & 0xFF) ? 1 : -1;
    motion_jog(axis, jog_step * sign);
}

static void cb_home_x(lv_event_t *e) { motion_home_axis('X'); }
static void cb_home_y(lv_event_t *e) { motion_home_axis('Y'); }
static void cb_home_z(lv_event_t *e) { motion_home_axis('Z'); }
static void cb_home_all(lv_event_t *e) { motion_home_all(); }

static void cb_spindle_cw(lv_event_t *e) {
    int rpm = lv_slider_get_value(slider_spindle);
    motion_spindle(rpm, 1);
}
static void cb_spindle_ccw(lv_event_t *e) {
    int rpm = lv_slider_get_value(slider_spindle);
    motion_spindle(rpm, 2);
}
static void cb_spindle_stop(lv_event_t *e) {
    motion_spindle_stop();
}

static void cb_estop(lv_event_t *e) { motion_estop(); }

static void make_axis_row(lv_obj_t *scr, int y, char axis,
                           lv_obj_t **lbl_pos, lv_event_cb_t cb_home) {
    char buf[16];

    // Axis label
    snprintf(buf, sizeof(buf), "%c", axis);
    lv_obj_t *lbl_ax = ui_create_label(scr, buf, &lv_font_montserrat_28, COLOR_ACCENT);
    lv_obj_set_pos(lbl_ax, 20, y + 6);

    // Position readout
    *lbl_pos = ui_create_label(scr, "   0.000 mm", &lv_font_montserrat_20, COLOR_TEXT);
    lv_obj_set_pos(*lbl_pos, 55, y + 10);

    // Jog minus
    lv_obj_t *btn_m = ui_create_btn(scr, LV_SYMBOL_MINUS, 70, 48, COLOR_PRIMARY, cb_jog);
    lv_obj_set_pos(btn_m, 220, y);
    intptr_t code_m = ((intptr_t)axis << 8) | 0;
    lv_obj_set_user_data(btn_m, (void *)code_m);
    lv_obj_remove_event_cb(btn_m, nullptr);
    lv_obj_add_event_cb(btn_m, cb_jog, LV_EVENT_CLICKED, (void *)code_m);

    // Jog plus
    lv_obj_t *btn_p = ui_create_btn(scr, LV_SYMBOL_PLUS, 70, 48, COLOR_PRIMARY, cb_jog);
    lv_obj_set_pos(btn_p, 296, y);
    intptr_t code_p = ((intptr_t)axis << 8) | 1;
    lv_obj_set_user_data(btn_p, (void *)code_p);
    lv_obj_remove_event_cb(btn_p, nullptr);
    lv_obj_add_event_cb(btn_p, cb_jog, LV_EVENT_CLICKED, (void *)code_p);

    // Home button
    lv_obj_t *btn_h = ui_create_btn(scr, LV_SYMBOL_HOME, 54, 48, COLOR_ORANGE, cb_home);
    lv_obj_set_pos(btn_h, 376, y);
}

lv_obj_t *ui_manual_create(lv_obj_t *parent) {
    lv_obj_t *scr = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(scr, COLOR_BG, 0);

    // Title bar
    lv_obj_t *bar = lv_obj_create(scr);
    lv_obj_set_size(bar, 800, 46);
    lv_obj_set_pos(bar, 0, 0);
    lv_obj_set_style_bg_color(bar, COLOR_PRIMARY, 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_radius(bar, 0, 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *btn_back = ui_create_btn(bar, LV_SYMBOL_LEFT, 50, 34, COLOR_PRIMARY, cb_back);
    lv_obj_align(btn_back, LV_ALIGN_LEFT_MID, 4, 0);

    ui_create_label(bar, "MANUAL CONTROL", &lv_font_montserrat_20, lv_color_white());
    lv_obj_align(lv_obj_get_child(bar, -1), LV_ALIGN_LEFT_MID, 60, 0);

    // Axis rows
    make_axis_row(scr, 56, 'X', &lbl_x_pos, cb_home_x);
    make_axis_row(scr, 112, 'Y', &lbl_y_pos, cb_home_y);
    make_axis_row(scr, 168, 'Z', &lbl_z_pos, cb_home_z);

    // Home all
    ui_create_btn(scr, LV_SYMBOL_HOME " ALL", 100, 48, COLOR_ACCENT, cb_home_all);
    lv_obj_set_pos(lv_obj_get_child(scr, -1), 20, 230);

    // Step size buttons
    ui_create_label(scr, "Step:", &lv_font_montserrat_14, COLOR_TEXT_DIM);
    lv_obj_set_pos(lv_obj_get_child(scr, -1), 140, 242);

    btns_step[0] = ui_create_btn(scr, "0.1", 60, 36, COLOR_PRIMARY, cb_step_01);
    lv_obj_set_pos(btns_step[0], 184, 234);
    btns_step[1] = ui_create_btn(scr, "1.0", 60, 36, COLOR_ACCENT, cb_step_1);
    lv_obj_set_pos(btns_step[1], 250, 234);
    btns_step[2] = ui_create_btn(scr, "10", 60, 36, COLOR_PRIMARY, cb_step_10);
    lv_obj_set_pos(btns_step[2], 316, 234);
    btns_step[3] = ui_create_btn(scr, "100", 60, 36, COLOR_PRIMARY, cb_step_100);
    lv_obj_set_pos(btns_step[3], 382, 234);

    // --- Spindle control (right side) ---
    lv_obj_t *card_sp = ui_create_card(scr, 340, 220);
    lv_obj_set_pos(card_sp, 448, 56);

    ui_create_label(card_sp, "Spindle", &lv_font_montserrat_16, COLOR_TEXT_DIM);

    lbl_spindle_rpm = ui_create_label(card_sp, "0 RPM", &lv_font_montserrat_36, COLOR_ACCENT);
    lv_obj_set_pos(lbl_spindle_rpm, 12, 28);

    ui_create_label(card_sp, "Speed", &lv_font_montserrat_12, COLOR_TEXT_DIM);
    lv_obj_set_pos(lv_obj_get_child(card_sp, -1), 12, 80);

    slider_spindle = lv_slider_create(card_sp);
    lv_obj_set_size(slider_spindle, 290, 20);
    lv_obj_set_pos(slider_spindle, 12, 100);
    lv_slider_set_range(slider_spindle, 0, 600);
    lv_slider_set_value(slider_spindle, 200, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(slider_spindle, COLOR_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider_spindle, COLOR_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider_spindle, lv_color_white(), LV_PART_KNOB);

    lv_obj_t *btn_ccw = ui_create_btn(card_sp, "CCW", 90, 50, COLOR_PRIMARY, cb_spindle_ccw);
    lv_obj_set_pos(btn_ccw, 12, 135);

    lv_obj_t *btn_stop = ui_create_btn(card_sp, "STOP", 90, 50, COLOR_RED, cb_spindle_stop);
    lv_obj_set_pos(btn_stop, 112, 135);

    lv_obj_t *btn_cw = ui_create_btn(card_sp, "CW", 90, 50, COLOR_GREEN, cb_spindle_cw);
    lv_obj_set_pos(btn_cw, 212, 135);

    // E-STOP
    lv_obj_t *btn_es = ui_create_btn(scr, "E-STOP", 180, 80, COLOR_RED, cb_estop);
    lv_obj_set_pos(btn_es, 20, 385);
    lv_obj_set_style_text_font(lv_obj_get_child(btn_es, 0), &lv_font_montserrat_24, 0);

    return scr;
}

void ui_manual_update() {
    const MachineState &ms = motion_get_state();
    char buf[32];

    snprintf(buf, sizeof(buf), "%8.3f mm", ms.pos_x);
    lv_label_set_text(lbl_x_pos, buf);
    snprintf(buf, sizeof(buf), "%8.3f mm", ms.pos_y);
    lv_label_set_text(lbl_y_pos, buf);
    snprintf(buf, sizeof(buf), "%8.3f mm", ms.pos_z);
    lv_label_set_text(lbl_z_pos, buf);

    snprintf(buf, sizeof(buf), "%.0f RPM", ms.spindle_rpm);
    lv_label_set_text(lbl_spindle_rpm, buf);
}
