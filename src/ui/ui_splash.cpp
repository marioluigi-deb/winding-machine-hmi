#include "ui.h"
#include <stdio.h>
#include <Arduino.h>

static lv_obj_t *lbl_title;
static lv_obj_t *lbl_sub;
static lv_obj_t *line_top;
static lv_obj_t *line_bot;
static unsigned long splash_start = 0;
static bool splash_boot = true;

static void cb_tap(lv_event_t *e) {
    ui_navigate(Screen::HOME);
}

lv_obj_t *ui_splash_create(lv_obj_t *parent) {
    lv_obj_t *scr = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x050508), 0);
    lv_obj_add_event_cb(scr, cb_tap, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_flag(scr, LV_OBJ_FLAG_CLICKABLE);

    // Top accent line
    static lv_point_t top_pts[] = {{200, 0}, {600, 0}};
    line_top = lv_line_create(scr);
    lv_line_set_points(line_top, top_pts, 2);
    lv_obj_set_style_line_color(line_top, COLOR_ACCENT, 0);
    lv_obj_set_style_line_width(line_top, 2, 0);
    lv_obj_set_pos(line_top, 0, 160);

    // "AP" — large
    lv_obj_t *lbl_ap = lv_label_create(scr);
    lv_label_set_text(lbl_ap, "AP");
    lv_obj_set_style_text_color(lbl_ap, COLOR_ACCENT, 0);
    lv_obj_set_style_text_font(lbl_ap, &lv_font_montserrat_48, 0);
    lv_obj_align(lbl_ap, LV_ALIGN_CENTER, -120, -20);

    // "WINDER" — large
    lbl_title = lv_label_create(scr);
    lv_label_set_text(lbl_title, "WINDER");
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xE0E0E0), 0);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_48, 0);
    lv_obj_align(lbl_title, LV_ALIGN_CENTER, 40, -20);

    // Bottom accent line
    static lv_point_t bot_pts[] = {{200, 0}, {600, 0}};
    line_bot = lv_line_create(scr);
    lv_line_set_points(line_bot, bot_pts, 2);
    lv_obj_set_style_line_color(line_bot, COLOR_ACCENT, 0);
    lv_obj_set_style_line_width(line_bot, 2, 0);
    lv_obj_set_pos(line_bot, 0, 310);

    // Subtitle
    lbl_sub = lv_label_create(scr);
    lv_label_set_text(lbl_sub, "MOTOR WINDING SYSTEM");
    lv_obj_set_style_text_color(lbl_sub, COLOR_TEXT_DIM, 0);
    lv_obj_set_style_text_font(lbl_sub, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_letter_space(lbl_sub, 6, 0);
    lv_obj_align(lbl_sub, LV_ALIGN_CENTER, 0, 40);

    // "Tap to continue" hint
    lv_obj_t *lbl_hint = lv_label_create(scr);
    lv_label_set_text(lbl_hint, "tap to continue");
    lv_obj_set_style_text_color(lbl_hint, lv_color_hex(0x444444), 0);
    lv_obj_set_style_text_font(lbl_hint, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_hint, LV_ALIGN_BOTTOM_MID, 0, -20);

    return scr;
}

void ui_splash_update() {
    // Auto-dismiss after 3 seconds on boot
    if (splash_boot && splash_start > 0 && millis() - splash_start > 3000) {
        splash_boot = false;
        ui_navigate(Screen::HOME);
    }
}

// Called from ui.cpp when navigating to splash
void ui_splash_start() {
    splash_start = millis();
}
