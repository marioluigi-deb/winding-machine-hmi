#include "ui.h"
#include "../motion.h"
#include <stdio.h>

static lv_obj_t *lbl_status;
static lv_obj_t *led_status;
static lv_obj_t *lbl_pos_x, *lbl_pos_y, *lbl_pos_z;
static lv_obj_t *lbl_program_name;
static lv_obj_t *lbl_turns_info;
static lv_obj_t *lbl_conn;

static void cb_setup(lv_event_t *e)   { ui_navigate(Screen::SETUP); }
static void cb_manual(lv_event_t *e)  { ui_navigate(Screen::MANUAL); }
static void cb_run(lv_event_t *e)     { ui_navigate(Screen::RUN); }
static void cb_settings(lv_event_t *e){ ui_navigate(Screen::SETTINGS); }

static void cb_home_all(lv_event_t *e) {
    motion_home_all();
}

static void cb_estop(lv_event_t *e) {
    motion_estop();
}

lv_obj_t *ui_home_create(lv_obj_t *parent) {
    lv_obj_t *scr = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(scr, COLOR_BG, 0);

    // --- Title bar ---
    lv_obj_t *title_bar = lv_obj_create(scr);
    lv_obj_set_size(title_bar, 800, 50);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_set_style_bg_color(title_bar, COLOR_PRIMARY, 0);
    lv_obj_set_style_bg_opa(title_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(title_bar, 0, 0);
    lv_obj_set_style_radius(title_bar, 0, 0);
    lv_obj_set_style_pad_hor(title_bar, 16, 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_title = ui_create_label(title_bar, "WINDING MACHINE",
                                           &lv_font_montserrat_24, lv_color_white());
    lv_obj_align(lbl_title, LV_ALIGN_LEFT_MID, 0, 0);

    lbl_conn = ui_create_label(title_bar, "DISCONNECTED",
                               &lv_font_montserrat_14, COLOR_TEXT_DIM);
    lv_obj_align(lbl_conn, LV_ALIGN_RIGHT_MID, 0, 0);

    // --- Status card ---
    lv_obj_t *card_status = ui_create_card(scr, 370, 160);
    lv_obj_set_pos(card_status, 15, 60);

    ui_create_label(card_status, "Machine Status", &lv_font_montserrat_14, COLOR_TEXT_DIM);

    led_status = lv_led_create(card_status);
    lv_obj_set_size(led_status, 16, 16);
    lv_obj_set_pos(led_status, 12, 38);
    lv_led_set_color(led_status, COLOR_GREEN);
    lv_led_on(led_status);

    lbl_status = ui_create_label(card_status, "IDLE", &lv_font_montserrat_28, COLOR_GREEN);
    lv_obj_set_pos(lbl_status, 36, 30);

    // Position readouts
    lbl_pos_x = ui_create_label(card_status, "X:   0.000 mm", &lv_font_montserrat_16, COLOR_TEXT);
    lv_obj_set_pos(lbl_pos_x, 12, 75);

    lbl_pos_y = ui_create_label(card_status, "Y:   0.000 mm", &lv_font_montserrat_16, COLOR_TEXT);
    lv_obj_set_pos(lbl_pos_y, 12, 98);

    lbl_pos_z = ui_create_label(card_status, "Z:   0.000 mm", &lv_font_montserrat_16, COLOR_TEXT);
    lv_obj_set_pos(lbl_pos_z, 12, 121);

    // --- Program card ---
    lv_obj_t *card_prog = ui_create_card(scr, 390, 160);
    lv_obj_set_pos(card_prog, 395, 60);

    ui_create_label(card_prog, "Active Program", &lv_font_montserrat_14, COLOR_TEXT_DIM);

    lbl_program_name = ui_create_label(card_prog, "Default", &lv_font_montserrat_24, COLOR_ACCENT);
    lv_obj_set_pos(lbl_program_name, 12, 30);

    lbl_turns_info = ui_create_label(card_prog, "200 turns / 2 layers / 24 AWG",
                                      &lv_font_montserrat_16, COLOR_TEXT);
    lv_obj_set_pos(lbl_turns_info, 12, 68);

    lv_obj_t *lbl_speed = ui_create_label(card_prog, "200 RPM  |  CW",
                                           &lv_font_montserrat_16, COLOR_TEXT);
    lv_obj_set_pos(lbl_speed, 12, 95);

    lv_obj_t *lbl_bobbin = ui_create_label(card_prog, "Bobbin: 50.0 x 25.0 mm",
                                            &lv_font_montserrat_14, COLOR_TEXT_DIM);
    lv_obj_set_pos(lbl_bobbin, 12, 122);

    // --- Button row ---
    int btn_y = 235;
    int btn_w = 180;
    int btn_h = 70;
    int gap = 12;
    int x0 = 15;

    lv_obj_t *btn_setup = ui_create_btn(scr, LV_SYMBOL_SETTINGS "  SETUP",
                                         btn_w, btn_h, COLOR_PRIMARY, cb_setup);
    lv_obj_set_pos(btn_setup, x0, btn_y);

    lv_obj_t *btn_manual = ui_create_btn(scr, LV_SYMBOL_SHUFFLE "  MANUAL",
                                          btn_w, btn_h, COLOR_PRIMARY, cb_manual);
    lv_obj_set_pos(btn_manual, x0 + btn_w + gap, btn_y);

    lv_obj_t *btn_run = ui_create_btn(scr, LV_SYMBOL_PLAY "  RUN",
                                       btn_w, btn_h, COLOR_GREEN, cb_run);
    lv_obj_set_pos(btn_run, x0 + 2 * (btn_w + gap), btn_y);

    lv_obj_t *btn_settings = ui_create_btn(scr, LV_SYMBOL_LIST "  CONFIG",
                                            btn_w, btn_h, COLOR_PRIMARY, cb_settings);
    lv_obj_set_pos(btn_settings, x0 + 3 * (btn_w + gap), btn_y);

    // --- Bottom row ---
    int bot_y = 320;

    lv_obj_t *btn_home = ui_create_btn(scr, LV_SYMBOL_HOME "  HOME ALL",
                                        240, 60, COLOR_ACCENT, cb_home_all);
    lv_obj_set_pos(btn_home, 15, bot_y);

    // E-STOP
    lv_obj_t *btn_estop = ui_create_btn(scr, "E-STOP", 240, 120, COLOR_RED, cb_estop);
    lv_obj_set_pos(btn_estop, 545, bot_y);
    lv_obj_set_style_text_font(lv_obj_get_child(btn_estop, 0), &lv_font_montserrat_28, 0);

    return scr;
}

void ui_home_update() {
    const MachineState &ms = motion_get_state();
    char buf[64];

    lv_label_set_text(lbl_status, machine_status_str(ms.status));
    lv_obj_set_style_text_color(lbl_status, machine_status_color(ms.status), 0);
    lv_led_set_color(led_status, machine_status_color(ms.status));

    if (ms.status == MachineStatus::DISCONNECTED) {
        lv_label_set_text(lbl_conn, "DISCONNECTED");
        lv_obj_set_style_text_color(lbl_conn, COLOR_RED, 0);
    } else {
        lv_label_set_text(lbl_conn, "CONNECTED");
        lv_obj_set_style_text_color(lbl_conn, COLOR_GREEN, 0);
    }

    snprintf(buf, sizeof(buf), "X: %8.3f mm", ms.pos_x);
    lv_label_set_text(lbl_pos_x, buf);
    snprintf(buf, sizeof(buf), "Y: %8.3f mm", ms.pos_y);
    lv_label_set_text(lbl_pos_y, buf);
    snprintf(buf, sizeof(buf), "Z: %8.3f mm", ms.pos_z);
    lv_label_set_text(lbl_pos_z, buf);

    lv_label_set_text(lbl_program_name, ui_program.name);
    snprintf(buf, sizeof(buf), "%d turns / %d layers / %d AWG",
             ui_program.total_turns, ui_program.num_layers, ui_program.awg);
    lv_label_set_text(lbl_turns_info, buf);
}
