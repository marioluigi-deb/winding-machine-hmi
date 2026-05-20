#include "ui.h"
#include "../motion.h"
#include "../storage.h"
#include <stdio.h>

// AWG to wire OD (mm) including typical insulation
static const float awg_od_mm[] = {
    // AWG 10-40 (index 0 = AWG10)
    2.74f, 2.45f, 2.19f, 1.96f, 1.75f,  // 10-14
    1.57f, 1.40f, 1.25f, 1.12f, 1.00f,  // 15-19
    0.89f, 0.80f, 0.71f, 0.64f, 0.57f,  // 20-24
    0.51f, 0.46f, 0.41f, 0.36f, 0.32f,  // 25-29
    0.29f, 0.26f, 0.23f, 0.21f, 0.19f,  // 30-34
    0.17f, 0.15f, 0.14f, 0.12f, 0.11f,  // 35-39
    0.10f                                  // 40
};

static lv_obj_t *dd_awg;
static lv_obj_t *spin_turns, *spin_layers;
static lv_obj_t *spin_bobbin_w, *spin_bobbin_d;
static lv_obj_t *spin_speed;
static lv_obj_t *sw_direction;
static lv_obj_t *lbl_wire_od;
static lv_obj_t *dd_slot;
static lv_obj_t *ta_name;

static void awg_to_od() {
    int sel = lv_dropdown_get_selected(dd_awg);
    int awg = sel + 10;
    ui_program.awg = awg;
    ui_program.wire_od_mm = (sel < 31) ? awg_od_mm[sel] : 0.10f;
    char buf[32];
    snprintf(buf, sizeof(buf), "Wire OD: %.3f mm", ui_program.wire_od_mm);
    lv_label_set_text(lbl_wire_od, buf);
}

static void cb_awg_change(lv_event_t *e) { awg_to_od(); }

static void cb_dir_change(lv_event_t *e) {
    ui_program.direction_cw = lv_obj_has_state(sw_direction, LV_STATE_CHECKED);
}

static void sync_program_from_ui() {
    lv_textarea_get_text(ta_name);
    strncpy(ui_program.name, lv_textarea_get_text(ta_name), sizeof(ui_program.name) - 1);
    ui_program.total_turns     = lv_spinbox_get_value(spin_turns);
    ui_program.num_layers      = lv_spinbox_get_value(spin_layers);
    ui_program.bobbin_width_mm = lv_spinbox_get_value(spin_bobbin_w) / 10.0f;
    ui_program.bobbin_id_mm    = lv_spinbox_get_value(spin_bobbin_d) / 10.0f;
    ui_program.speed_rpm       = lv_spinbox_get_value(spin_speed);
    ui_program.layer_step_mm   = ui_program.wire_od_mm;
    ui_program.traverse_start_mm = 0.0f;
    awg_to_od();
}

static void cb_save(lv_event_t *e) {
    sync_program_from_ui();
    int slot = lv_dropdown_get_selected(dd_slot);
    storage_save_program(slot, ui_program);
}

static void cb_load(lv_event_t *e) {
    int slot = lv_dropdown_get_selected(dd_slot);
    if (storage_load_program(slot, ui_program)) {
        lv_textarea_set_text(ta_name, ui_program.name);
        lv_spinbox_set_value(spin_turns, ui_program.total_turns);
        lv_spinbox_set_value(spin_layers, ui_program.num_layers);
        lv_spinbox_set_value(spin_bobbin_w, (int)(ui_program.bobbin_width_mm * 10));
        lv_spinbox_set_value(spin_bobbin_d, (int)(ui_program.bobbin_id_mm * 10));
        lv_spinbox_set_value(spin_speed, (int)ui_program.speed_rpm);
        lv_dropdown_set_selected(dd_awg, ui_program.awg - 10);
        if (ui_program.direction_cw)
            lv_obj_add_state(sw_direction, LV_STATE_CHECKED);
        else
            lv_obj_clear_state(sw_direction, LV_STATE_CHECKED);
        awg_to_od();
    }
}

static void cb_start(lv_event_t *e) {
    sync_program_from_ui();
    motion_wind_load(ui_program);
    ui_navigate(Screen::RUN);
}

static void cb_back(lv_event_t *e) { ui_navigate(Screen::HOME); }

static lv_obj_t *make_spinbox(lv_obj_t *parent, int x, int y, int w,
                               int range_min, int range_max, int digits,
                               int sep_pos, int initial) {
    lv_obj_t *sb = lv_spinbox_create(parent);
    lv_spinbox_set_range(sb, range_min, range_max);
    lv_spinbox_set_digit_format(sb, digits, sep_pos);
    lv_spinbox_set_value(sb, initial);
    lv_obj_set_size(sb, w, 40);
    lv_obj_set_pos(sb, x, y);
    lv_obj_set_style_bg_color(sb, COLOR_CARD, 0);
    lv_obj_set_style_text_color(sb, COLOR_TEXT, 0);
    lv_obj_set_style_border_color(sb, COLOR_ACCENT, 0);
    lv_obj_set_style_text_font(sb, &lv_font_montserrat_20, 0);

    lv_obj_t *btn_dec = lv_btn_create(parent);
    lv_obj_set_size(btn_dec, 36, 40);
    lv_obj_set_pos(btn_dec, x - 38, y);
    lv_obj_set_style_bg_color(btn_dec, COLOR_PRIMARY, 0);
    lv_obj_t *lbl_m = lv_label_create(btn_dec);
    lv_label_set_text(lbl_m, LV_SYMBOL_MINUS);
    lv_obj_center(lbl_m);
    lv_obj_add_event_cb(btn_dec, [](lv_event_t *ev) {
        lv_obj_t *s = (lv_obj_t *)lv_event_get_user_data(ev);
        lv_spinbox_decrement(s);
    }, LV_EVENT_CLICKED, sb);

    lv_obj_t *btn_inc = lv_btn_create(parent);
    lv_obj_set_size(btn_inc, 36, 40);
    lv_obj_set_pos(btn_inc, x + w + 2, y);
    lv_obj_set_style_bg_color(btn_inc, COLOR_PRIMARY, 0);
    lv_obj_t *lbl_p = lv_label_create(btn_inc);
    lv_label_set_text(lbl_p, LV_SYMBOL_PLUS);
    lv_obj_center(lbl_p);
    lv_obj_set_user_data(btn_inc, sb);
    lv_obj_add_event_cb(btn_inc, [](lv_event_t *ev) {
        lv_obj_t *s = (lv_obj_t *)lv_event_get_user_data(ev);
        lv_spinbox_increment(s);
    }, LV_EVENT_CLICKED, sb);

    return sb;
}

lv_obj_t *ui_setup_create(lv_obj_t *parent) {
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

    lv_obj_t *lbl_t = ui_create_label(bar, "PROGRAM SETUP", &lv_font_montserrat_20, lv_color_white());
    lv_obj_align(lbl_t, LV_ALIGN_LEFT_MID, 60, 0);

    // --- Left column: winding params ---
    int lx = 20, ly = 56;
    int row_h = 48;
    int label_w = 130;

    lv_obj_t *lbl_name = ui_create_label(scr, "Program Name", &lv_font_montserrat_14, COLOR_TEXT_DIM);
    lv_obj_set_pos(lbl_name, lx, ly);
    ta_name = lv_textarea_create(scr);
    lv_textarea_set_text(ta_name, ui_program.name);
    lv_textarea_set_one_line(ta_name, true);
    lv_textarea_set_max_length(ta_name, 30);
    lv_obj_set_size(ta_name, 200, 38);
    lv_obj_set_pos(ta_name, lx + label_w, ly - 4);
    lv_obj_set_style_bg_color(ta_name, COLOR_CARD, 0);
    lv_obj_set_style_text_color(ta_name, COLOR_TEXT, 0);
    lv_obj_set_style_border_color(ta_name, COLOR_ACCENT, 0);
    ly += row_h;

    ui_create_label(scr, "Wire Gauge", &lv_font_montserrat_14, COLOR_TEXT_DIM);
    lv_obj_set_pos(lv_obj_get_child(scr, -1), lx, ly + 8);
    dd_awg = lv_dropdown_create(scr);
    lv_dropdown_set_options(dd_awg,
        "10 AWG\n11 AWG\n12 AWG\n13 AWG\n14 AWG\n15 AWG\n16 AWG\n17 AWG\n18 AWG\n"
        "19 AWG\n20 AWG\n21 AWG\n22 AWG\n23 AWG\n24 AWG\n25 AWG\n26 AWG\n27 AWG\n"
        "28 AWG\n29 AWG\n30 AWG\n31 AWG\n32 AWG\n33 AWG\n34 AWG\n35 AWG\n36 AWG\n"
        "37 AWG\n38 AWG\n39 AWG\n40 AWG");
    lv_dropdown_set_selected(dd_awg, ui_program.awg - 10);
    lv_obj_set_size(dd_awg, 120, 38);
    lv_obj_set_pos(dd_awg, lx + label_w, ly + 2);
    lv_obj_set_style_bg_color(dd_awg, COLOR_CARD, 0);
    lv_obj_set_style_text_color(dd_awg, COLOR_TEXT, 0);
    lv_obj_add_event_cb(dd_awg, cb_awg_change, LV_EVENT_VALUE_CHANGED, nullptr);

    lbl_wire_od = ui_create_label(scr, "Wire OD: 0.570 mm", &lv_font_montserrat_14, COLOR_ACCENT);
    lv_obj_set_pos(lbl_wire_od, lx + label_w + 130, ly + 10);
    ly += row_h;

    ui_create_label(scr, "Total Turns", &lv_font_montserrat_14, COLOR_TEXT_DIM);
    lv_obj_set_pos(lv_obj_get_child(scr, -1), lx, ly + 8);
    spin_turns = make_spinbox(scr, lx + label_w + 38, ly + 2, 110,
                               1, 99999, 5, 0, ui_program.total_turns);
    ly += row_h;

    ui_create_label(scr, "Layers", &lv_font_montserrat_14, COLOR_TEXT_DIM);
    lv_obj_set_pos(lv_obj_get_child(scr, -1), lx, ly + 8);
    spin_layers = make_spinbox(scr, lx + label_w + 38, ly + 2, 70,
                                1, 99, 2, 0, ui_program.num_layers);
    ly += row_h;

    // --- Right column ---
    int rx = 420, ry = 56;

    ui_create_label(scr, "Bobbin Width (mm)", &lv_font_montserrat_14, COLOR_TEXT_DIM);
    lv_obj_set_pos(lv_obj_get_child(scr, -1), rx, ry + 8);
    spin_bobbin_w = make_spinbox(scr, rx + 170, ry + 2, 100,
                                  10, 9999, 4, 3, (int)(ui_program.bobbin_width_mm * 10));
    ry += row_h;

    ui_create_label(scr, "Bobbin ID (mm)", &lv_font_montserrat_14, COLOR_TEXT_DIM);
    lv_obj_set_pos(lv_obj_get_child(scr, -1), rx, ry + 8);
    spin_bobbin_d = make_spinbox(scr, rx + 170, ry + 2, 100,
                                  10, 9999, 4, 3, (int)(ui_program.bobbin_id_mm * 10));
    ry += row_h;

    ui_create_label(scr, "Speed (RPM)", &lv_font_montserrat_14, COLOR_TEXT_DIM);
    lv_obj_set_pos(lv_obj_get_child(scr, -1), rx, ry + 8);
    spin_speed = make_spinbox(scr, rx + 170, ry + 2, 100,
                               10, 2000, 4, 0, (int)ui_program.speed_rpm);
    ry += row_h;

    ui_create_label(scr, "Direction", &lv_font_montserrat_14, COLOR_TEXT_DIM);
    lv_obj_set_pos(lv_obj_get_child(scr, -1), rx, ry + 8);
    sw_direction = lv_switch_create(scr);
    lv_obj_set_pos(sw_direction, rx + 170, ry + 4);
    if (ui_program.direction_cw) lv_obj_add_state(sw_direction, LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw_direction, cb_dir_change, LV_EVENT_VALUE_CHANGED, nullptr);
    ui_create_label(scr, "CCW", &lv_font_montserrat_14, COLOR_TEXT_DIM);
    lv_obj_set_pos(lv_obj_get_child(scr, -1), rx + 136, ry + 10);
    ui_create_label(scr, "CW", &lv_font_montserrat_14, COLOR_TEXT_DIM);
    lv_obj_set_pos(lv_obj_get_child(scr, -1), rx + 225, ry + 10);

    // --- Bottom buttons ---
    int by = 420;

    // Slot selector
    dd_slot = lv_dropdown_create(scr);
    lv_dropdown_set_options(dd_slot, "Slot 1\nSlot 2\nSlot 3\nSlot 4\nSlot 5\nSlot 6\nSlot 7\nSlot 8");
    lv_obj_set_size(dd_slot, 120, 42);
    lv_obj_set_pos(dd_slot, 20, by);
    lv_obj_set_style_bg_color(dd_slot, COLOR_CARD, 0);
    lv_obj_set_style_text_color(dd_slot, COLOR_TEXT, 0);

    ui_create_btn(scr, LV_SYMBOL_DOWNLOAD " LOAD", 110, 42, COLOR_PRIMARY, cb_load);
    lv_obj_set_pos(lv_obj_get_child(scr, -1), 150, by);

    ui_create_btn(scr, LV_SYMBOL_SAVE " SAVE", 110, 42, COLOR_PRIMARY, cb_save);
    lv_obj_set_pos(lv_obj_get_child(scr, -1), 270, by);

    ui_create_btn(scr, LV_SYMBOL_PLAY "  START WINDING", 240, 42, COLOR_GREEN, cb_start);
    lv_obj_set_pos(lv_obj_get_child(scr, -1), 540, by);

    awg_to_od();
    return scr;
}

void ui_setup_update() {
    // Nothing dynamic here — form is user-driven
}
