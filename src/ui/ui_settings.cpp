#include "ui.h"
#include "../motion.h"
#include "../storage.h"

static lv_obj_t *spins[11];  // all config spinboxes
static lv_obj_t *lbl_fw_ver;

enum CfgIdx {
    SPM_X = 0, SPM_Y, SPM_Z, SPR,
    MF_X, MF_Y, MF_Z,
    ACC_X, ACC_Y, ACC_Z,
    MAX_RPM
};

static const char *cfg_labels[] = {
    "Steps/mm X", "Steps/mm Y", "Steps/mm Z", "Steps/rev",
    "Max Feed X", "Max Feed Y", "Max Feed Z",
    "Accel X", "Accel Y", "Accel Z",
    "Max Spindle RPM"
};

static void cb_back(lv_event_t *e) { ui_navigate(Screen::HOME); }

static void sync_config_from_ui() {
    ui_config.steps_per_mm_x  = lv_spinbox_get_value(spins[SPM_X]);
    ui_config.steps_per_mm_y  = lv_spinbox_get_value(spins[SPM_Y]);
    ui_config.steps_per_mm_z  = lv_spinbox_get_value(spins[SPM_Z]);
    ui_config.steps_per_rev   = lv_spinbox_get_value(spins[SPR]);
    ui_config.max_feed_x      = lv_spinbox_get_value(spins[MF_X]);
    ui_config.max_feed_y      = lv_spinbox_get_value(spins[MF_Y]);
    ui_config.max_feed_z      = lv_spinbox_get_value(spins[MF_Z]);
    ui_config.accel_x         = lv_spinbox_get_value(spins[ACC_X]);
    ui_config.accel_y         = lv_spinbox_get_value(spins[ACC_Y]);
    ui_config.accel_z         = lv_spinbox_get_value(spins[ACC_Z]);
    ui_config.max_spindle_rpm = lv_spinbox_get_value(spins[MAX_RPM]);
}

static void cb_save(lv_event_t *e) {
    sync_config_from_ui();
    storage_save_config(ui_config);

    // Push to motion controller
    motion_set_config("SPM_X", ui_config.steps_per_mm_x);
    motion_set_config("SPM_Y", ui_config.steps_per_mm_y);
    motion_set_config("SPM_Z", ui_config.steps_per_mm_z);
    motion_set_config("SPR",   ui_config.steps_per_rev);
    motion_set_config("MF_X",  ui_config.max_feed_x);
    motion_set_config("MF_Y",  ui_config.max_feed_y);
    motion_set_config("MF_Z",  ui_config.max_feed_z);
    motion_set_config("ACC_X", ui_config.accel_x);
    motion_set_config("ACC_Y", ui_config.accel_y);
    motion_set_config("ACC_Z", ui_config.accel_z);
    motion_set_config("MSR",   ui_config.max_spindle_rpm);

    // Feedback
    lv_obj_t *mbox = lv_msgbox_create(nullptr, "Saved",
        "Configuration saved to flash and sent to controller.", nullptr, true);
    lv_obj_center(mbox);
}

static void cb_reset_ctrl(lv_event_t *e) {
    motion_reset();
}

static lv_obj_t *make_cfg_spin(lv_obj_t *parent, int x, int y,
                                int val, int range_max) {
    lv_obj_t *sb = lv_spinbox_create(parent);
    lv_spinbox_set_range(sb, 1, range_max);
    lv_spinbox_set_digit_format(sb, 5, 0);
    lv_spinbox_set_value(sb, val);
    lv_obj_set_size(sb, 100, 34);
    lv_obj_set_pos(sb, x, y);
    lv_obj_set_style_bg_color(sb, COLOR_CARD, 0);
    lv_obj_set_style_text_color(sb, COLOR_TEXT, 0);
    lv_obj_set_style_border_color(sb, COLOR_ACCENT, 0);
    lv_obj_set_style_text_font(sb, &lv_font_montserrat_14, 0);

    lv_obj_t *bm = lv_btn_create(parent);
    lv_obj_set_size(bm, 30, 34);
    lv_obj_set_pos(bm, x - 32, y);
    lv_obj_set_style_bg_color(bm, COLOR_PRIMARY, 0);
    lv_obj_t *lm = lv_label_create(bm);
    lv_label_set_text(lm, LV_SYMBOL_MINUS);
    lv_obj_center(lm);
    lv_obj_set_user_data(bm, sb);
    lv_obj_add_event_cb(bm, [](lv_event_t *ev) {
        lv_spinbox_decrement((lv_obj_t *)lv_event_get_user_data(ev));
    }, LV_EVENT_CLICKED, sb);

    lv_obj_t *bp = lv_btn_create(parent);
    lv_obj_set_size(bp, 30, 34);
    lv_obj_set_pos(bp, x + 102, y);
    lv_obj_set_style_bg_color(bp, COLOR_PRIMARY, 0);
    lv_obj_t *lp = lv_label_create(bp);
    lv_label_set_text(lp, LV_SYMBOL_PLUS);
    lv_obj_center(lp);
    lv_obj_set_user_data(bp, sb);
    lv_obj_add_event_cb(bp, [](lv_event_t *ev) {
        lv_spinbox_increment((lv_obj_t *)lv_event_get_user_data(ev));
    }, LV_EVENT_CLICKED, sb);

    return sb;
}

lv_obj_t *ui_settings_create(lv_obj_t *parent) {
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

    ui_create_label(bar, "MACHINE SETTINGS", &lv_font_montserrat_20, lv_color_white());
    lv_obj_align(lv_obj_get_child(bar, -1), LV_ALIGN_LEFT_MID, 60, 0);

    // Scrollable container for settings
    lv_obj_t *cont = lv_obj_create(scr);
    lv_obj_set_size(cont, 780, 370);
    lv_obj_set_pos(cont, 10, 50);
    lv_obj_set_style_bg_color(cont, COLOR_BG, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 4, 0);
    lv_obj_set_scroll_dir(cont, LV_DIR_VER);

    float vals[] = {
        ui_config.steps_per_mm_x, ui_config.steps_per_mm_y, ui_config.steps_per_mm_z,
        ui_config.steps_per_rev,
        ui_config.max_feed_x, ui_config.max_feed_y, ui_config.max_feed_z,
        ui_config.accel_x, ui_config.accel_y, ui_config.accel_z,
        ui_config.max_spindle_rpm
    };
    int maxvals[] = {
        50000, 50000, 50000, 50000,
        50000, 50000, 50000,
        50000, 50000, 50000,
        5000
    };

    int cols = 2;
    int col_w = 370;
    int row_h = 42;

    for (int i = 0; i < 11; i++) {
        int col = i % cols;
        int row = i / cols;
        int x = col * col_w + 140;
        int y = row * row_h + 4;

        lv_obj_t *lbl = ui_create_label(cont, cfg_labels[i],
                                         &lv_font_montserrat_14, COLOR_TEXT_DIM);
        lv_obj_set_pos(lbl, col * col_w + 4, y + 8);

        spins[i] = make_cfg_spin(cont, x, y, (int)vals[i], maxvals[i]);
    }

    // Bottom buttons
    int by = 430;

    ui_create_btn(scr, LV_SYMBOL_SAVE " SAVE CONFIG", 200, 42, COLOR_GREEN, cb_save);
    lv_obj_set_pos(lv_obj_get_child(scr, -1), 15, by);

    ui_create_btn(scr, LV_SYMBOL_REFRESH " RESET CTRL", 180, 42, COLOR_ORANGE, cb_reset_ctrl);
    lv_obj_set_pos(lv_obj_get_child(scr, -1), 230, by);

    ui_create_btn(scr, LV_SYMBOL_WIFI " WIFI", 120, 42, COLOR_ACCENT,
        [](lv_event_t *e) { ui_navigate(Screen::WIFI); });
    lv_obj_set_pos(lv_obj_get_child(scr, -1), 425, by);

    // Firmware version
    lbl_fw_ver = ui_create_label(scr, "Winding Machine HMI v2.0",
                                  &lv_font_montserrat_12, COLOR_TEXT_DIM);
    lv_obj_set_pos(lbl_fw_ver, 580, by + 14);

    return scr;
}

void ui_settings_update() {
    // Static screen — nothing to poll
}
