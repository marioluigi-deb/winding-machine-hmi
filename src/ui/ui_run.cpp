#include "ui.h"
#include "../motion.h"
#include <stdio.h>
#include <Arduino.h>

static lv_obj_t *lbl_turn_big;
static lv_obj_t *lbl_turn_total;
static lv_obj_t *lbl_layer;
static lv_obj_t *bar_progress;
static lv_obj_t *lbl_percent;
static lv_obj_t *lbl_speed;
static lv_obj_t *lbl_eta;
static lv_obj_t *lbl_run_status;
static lv_obj_t *lbl_pos_x, *lbl_pos_y, *lbl_pos_z;
static lv_obj_t *btn_pause, *btn_resume;
static unsigned long wind_start_ms = 0;

static void cb_back(lv_event_t *e) { ui_navigate(Screen::HOME); }

static void cb_pause(lv_event_t *e) {
    motion_wind_pause();
}

static void cb_resume(lv_event_t *e) {
    motion_wind_resume();
}

static void cb_abort(lv_event_t *e) {
    // Confirm dialog
    static const char *btns[] = {"Abort", "Cancel", ""};
    lv_obj_t *mbox = lv_msgbox_create(nullptr, "Abort Winding",
        "Stop the current winding operation?", btns, true);
    lv_obj_center(mbox);
    lv_obj_add_event_cb(mbox, [](lv_event_t *ev) {
        lv_obj_t *obj = lv_event_get_current_target(ev);
        const char *txt = lv_msgbox_get_active_btn_text(obj);
        if (txt && strcmp(txt, "Abort") == 0) {
            motion_wind_abort();
        }
        lv_msgbox_close(obj);
    }, LV_EVENT_VALUE_CHANGED, nullptr);
}

static void cb_start(lv_event_t *e) {
    motion_wind_load(ui_program);
    motion_wind_start();
    wind_start_ms = millis();
}

static void cb_speed_down(lv_event_t *e) {
    float spd = ui_program.speed_rpm - 25.0f;
    if (spd < 25.0f) spd = 25.0f;
    ui_program.speed_rpm = spd;
    motion_wind_speed(spd);
}

static void cb_speed_up(lv_event_t *e) {
    float spd = ui_program.speed_rpm + 25.0f;
    if (spd > ui_config.max_spindle_rpm) spd = ui_config.max_spindle_rpm;
    ui_program.speed_rpm = spd;
    motion_wind_speed(spd);
}

lv_obj_t *ui_run_create(lv_obj_t *parent) {
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

    ui_create_label(bar, "WINDING", &lv_font_montserrat_20, lv_color_white());
    lv_obj_align(lv_obj_get_child(bar, -1), LV_ALIGN_LEFT_MID, 60, 0);

    lbl_run_status = ui_create_label(bar, "IDLE", &lv_font_montserrat_16, COLOR_GREEN);
    lv_obj_align(lbl_run_status, LV_ALIGN_RIGHT_MID, -12, 0);

    // === Main turn counter card ===
    lv_obj_t *card_turns = ui_create_card(scr, 460, 180);
    lv_obj_set_pos(card_turns, 15, 56);

    ui_create_label(card_turns, "Turn", &lv_font_montserrat_14, COLOR_TEXT_DIM);

    lbl_turn_big = ui_create_label(card_turns, "0", &lv_font_montserrat_48, COLOR_GREEN);
    lv_obj_set_pos(lbl_turn_big, 12, 20);

    lbl_turn_total = ui_create_label(card_turns, "/ 200", &lv_font_montserrat_24, COLOR_TEXT_DIM);
    lv_obj_set_pos(lbl_turn_total, 12, 76);

    // Layer
    ui_create_label(card_turns, "Layer", &lv_font_montserrat_14, COLOR_TEXT_DIM);
    lv_obj_set_pos(lv_obj_get_child(card_turns, -1), 250, 0);

    lbl_layer = ui_create_label(card_turns, "1 / 2", &lv_font_montserrat_28, COLOR_ACCENT);
    lv_obj_set_pos(lbl_layer, 250, 22);

    // Progress bar
    bar_progress = lv_bar_create(card_turns);
    lv_obj_set_size(bar_progress, 420, 24);
    lv_obj_set_pos(bar_progress, 12, 118);
    lv_bar_set_range(bar_progress, 0, 1000);
    lv_bar_set_value(bar_progress, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_progress, COLOR_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_progress, COLOR_GREEN, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar_progress, 6, 0);
    lv_obj_set_style_radius(bar_progress, 6, LV_PART_INDICATOR);

    lbl_percent = ui_create_label(card_turns, "0%", &lv_font_montserrat_14, COLOR_TEXT);
    lv_obj_set_pos(lbl_percent, 12, 146);

    lbl_eta = ui_create_label(card_turns, "ETA: --:--", &lv_font_montserrat_14, COLOR_TEXT_DIM);
    lv_obj_set_pos(lbl_eta, 350, 146);

    // === Info card (right) ===
    lv_obj_t *card_info = ui_create_card(scr, 300, 180);
    lv_obj_set_pos(card_info, 485, 56);

    ui_create_label(card_info, "Speed", &lv_font_montserrat_14, COLOR_TEXT_DIM);
    lbl_speed = ui_create_label(card_info, "200 RPM", &lv_font_montserrat_28, COLOR_ACCENT);
    lv_obj_set_pos(lbl_speed, 12, 22);

    // Speed +/- buttons
    lv_obj_t *btn_sd = ui_create_btn(card_info, "-25", 60, 36, COLOR_PRIMARY, cb_speed_down);
    lv_obj_set_pos(btn_sd, 12, 62);
    lv_obj_t *btn_su = ui_create_btn(card_info, "+25", 60, 36, COLOR_PRIMARY, cb_speed_up);
    lv_obj_set_pos(btn_su, 80, 62);

    // Position
    lbl_pos_x = ui_create_label(card_info, "X:   0.000", &lv_font_montserrat_14, COLOR_TEXT);
    lv_obj_set_pos(lbl_pos_x, 12, 110);
    lbl_pos_y = ui_create_label(card_info, "Y:   0.000", &lv_font_montserrat_14, COLOR_TEXT);
    lv_obj_set_pos(lbl_pos_y, 12, 130);
    lbl_pos_z = ui_create_label(card_info, "Z:   0.000", &lv_font_montserrat_14, COLOR_TEXT);
    lv_obj_set_pos(lbl_pos_z, 12, 150);

    // === Bottom control buttons ===
    int by = 250;

    lv_obj_t *btn_start = ui_create_btn(scr, LV_SYMBOL_PLAY "  START", 160, 60, COLOR_GREEN, cb_start);
    lv_obj_set_pos(btn_start, 15, by);

    btn_pause = ui_create_btn(scr, LV_SYMBOL_PAUSE "  PAUSE", 160, 60, COLOR_ORANGE, cb_pause);
    lv_obj_set_pos(btn_pause, 185, by);

    btn_resume = ui_create_btn(scr, LV_SYMBOL_PLAY "  RESUME", 160, 60, COLOR_ACCENT, cb_resume);
    lv_obj_set_pos(btn_resume, 355, by);

    lv_obj_t *btn_abort = ui_create_btn(scr, LV_SYMBOL_CLOSE "  ABORT", 160, 60, COLOR_RED, cb_abort);
    lv_obj_set_pos(btn_abort, 625, by);

    return scr;
}

void ui_run_update() {
    const MachineState &ms = motion_get_state();
    char buf[64];

    // Status
    lv_label_set_text(lbl_run_status, machine_status_str(ms.status));
    lv_obj_set_style_text_color(lbl_run_status, machine_status_color(ms.status), 0);

    // Turn counter
    snprintf(buf, sizeof(buf), "%d", ms.current_turn);
    lv_label_set_text(lbl_turn_big, buf);

    snprintf(buf, sizeof(buf), "/ %d", ui_program.total_turns);
    lv_label_set_text(lbl_turn_total, buf);

    // Color based on progress
    if (ms.status == MachineStatus::WINDING) {
        lv_obj_set_style_text_color(lbl_turn_big, COLOR_GREEN, 0);
    } else if (ms.status == MachineStatus::PAUSED) {
        lv_obj_set_style_text_color(lbl_turn_big, COLOR_ORANGE, 0);
    }

    // Layer
    snprintf(buf, sizeof(buf), "%d / %d", ms.current_layer + 1, ui_program.num_layers);
    lv_label_set_text(lbl_layer, buf);

    // Progress
    int total = ui_program.total_turns;
    int pct_x10 = (total > 0) ? (ms.current_turn * 1000 / total) : 0;
    lv_bar_set_value(bar_progress, pct_x10, LV_ANIM_ON);
    snprintf(buf, sizeof(buf), "%d.%d%%", pct_x10 / 10, pct_x10 % 10);
    lv_label_set_text(lbl_percent, buf);

    // ETA
    if (ms.status == MachineStatus::WINDING && ms.current_turn > 0) {
        unsigned long elapsed_ms = millis() - wind_start_ms;
        float ms_per_turn = (float)elapsed_ms / ms.current_turn;
        int remaining_turns = total - ms.current_turn;
        int eta_sec = (int)(ms_per_turn * remaining_turns / 1000.0f);
        int m = eta_sec / 60;
        int s = eta_sec % 60;
        snprintf(buf, sizeof(buf), "ETA: %d:%02d", m, s);
    } else {
        snprintf(buf, sizeof(buf), "ETA: --:--");
    }
    lv_label_set_text(lbl_eta, buf);

    // Speed
    snprintf(buf, sizeof(buf), "%.0f RPM", ms.spindle_rpm);
    lv_label_set_text(lbl_speed, buf);

    // Positions
    snprintf(buf, sizeof(buf), "X: %7.3f", ms.pos_x);
    lv_label_set_text(lbl_pos_x, buf);
    snprintf(buf, sizeof(buf), "Y: %7.3f", ms.pos_y);
    lv_label_set_text(lbl_pos_y, buf);
    snprintf(buf, sizeof(buf), "Z: %7.3f", ms.pos_z);
    lv_label_set_text(lbl_pos_z, buf);

    // Show/hide pause vs resume
    if (ms.status == MachineStatus::PAUSED) {
        lv_obj_add_flag(btn_pause, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(btn_resume, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(btn_pause, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(btn_resume, LV_OBJ_FLAG_HIDDEN);
    }
}
