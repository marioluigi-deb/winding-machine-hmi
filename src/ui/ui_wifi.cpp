#include "ui.h"
#include "../wifi_manager.h"
#include "../storage.h"
#include <stdio.h>

static lv_obj_t *lbl_wifi_status;
static lv_obj_t *lbl_wifi_ip;
static lv_obj_t *lbl_wifi_rssi;
static lv_obj_t *ta_ssid;
static lv_obj_t *ta_pass;
static lv_obj_t *ta_hostname;
static lv_obj_t *ta_webhook;
static lv_obj_t *sw_notify_ok;
static lv_obj_t *sw_notify_err;
static lv_obj_t *kb;

static void cb_back(lv_event_t *e) { ui_navigate(Screen::SETTINGS); }

static void cb_ta_focused(lv_event_t *e) {
    lv_obj_t *ta = lv_event_get_target(e);
    lv_keyboard_set_textarea(kb, ta);
    lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
}

static void cb_kb_ready(lv_event_t *e) {
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
}

static void cb_connect(lv_event_t *e) {
    WiFiConfig &cfg = wifi_get_config();
    strlcpy(cfg.ssid, lv_textarea_get_text(ta_ssid), sizeof(cfg.ssid));
    strlcpy(cfg.password, lv_textarea_get_text(ta_pass), sizeof(cfg.password));
    strlcpy(cfg.hostname, lv_textarea_get_text(ta_hostname), sizeof(cfg.hostname));
    strlcpy(cfg.notify_url, lv_textarea_get_text(ta_webhook), sizeof(cfg.notify_url));
    cfg.notify_on_complete = lv_obj_has_state(sw_notify_ok, LV_STATE_CHECKED);
    cfg.notify_on_error = lv_obj_has_state(sw_notify_err, LV_STATE_CHECKED);
    cfg.auto_connect = true;
    storage_save_wifi(cfg);
    wifi_connect(cfg.ssid, cfg.password);
}

static void cb_start_ap(lv_event_t *e) {
    wifi_start_ap();
}

static void cb_disconnect(lv_event_t *e) {
    wifi_disconnect();
}

lv_obj_t *ui_wifi_create(lv_obj_t *parent) {
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

    ui_create_label(bar, LV_SYMBOL_WIFI "  WIFI SETTINGS", &lv_font_montserrat_20, lv_color_white());
    lv_obj_align(lv_obj_get_child(bar, -1), LV_ALIGN_LEFT_MID, 60, 0);

    // Status card
    lv_obj_t *card_st = ui_create_card(scr, 760, 50);
    lv_obj_set_pos(card_st, 20, 52);
    lbl_wifi_status = ui_create_label(card_st, "DISCONNECTED", &lv_font_montserrat_16, COLOR_RED);
    lv_obj_set_pos(lbl_wifi_status, 0, 2);
    lbl_wifi_ip = ui_create_label(card_st, "IP: --", &lv_font_montserrat_14, COLOR_TEXT_DIM);
    lv_obj_set_pos(lbl_wifi_ip, 250, 4);
    lbl_wifi_rssi = ui_create_label(card_st, "RSSI: --", &lv_font_montserrat_14, COLOR_TEXT_DIM);
    lv_obj_set_pos(lbl_wifi_rssi, 550, 4);

    // Left column: connection
    int lx = 20, ly = 110;

    ui_create_label(scr, "SSID", &lv_font_montserrat_14, COLOR_TEXT_DIM);
    lv_obj_set_pos(lv_obj_get_child(scr, -1), lx, ly);
    ta_ssid = lv_textarea_create(scr);
    lv_textarea_set_one_line(ta_ssid, true);
    lv_textarea_set_max_length(ta_ssid, 32);
    lv_obj_set_size(ta_ssid, 240, 36);
    lv_obj_set_pos(ta_ssid, lx + 80, ly - 4);
    lv_obj_set_style_bg_color(ta_ssid, COLOR_CARD, 0);
    lv_obj_set_style_text_color(ta_ssid, COLOR_TEXT, 0);
    lv_obj_add_event_cb(ta_ssid, cb_ta_focused, LV_EVENT_FOCUSED, nullptr);
    ly += 44;

    ui_create_label(scr, "Password", &lv_font_montserrat_14, COLOR_TEXT_DIM);
    lv_obj_set_pos(lv_obj_get_child(scr, -1), lx, ly);
    ta_pass = lv_textarea_create(scr);
    lv_textarea_set_one_line(ta_pass, true);
    lv_textarea_set_max_length(ta_pass, 64);
    lv_textarea_set_password_mode(ta_pass, true);
    lv_obj_set_size(ta_pass, 240, 36);
    lv_obj_set_pos(ta_pass, lx + 80, ly - 4);
    lv_obj_set_style_bg_color(ta_pass, COLOR_CARD, 0);
    lv_obj_set_style_text_color(ta_pass, COLOR_TEXT, 0);
    lv_obj_add_event_cb(ta_pass, cb_ta_focused, LV_EVENT_FOCUSED, nullptr);
    ly += 44;

    ui_create_label(scr, "Hostname", &lv_font_montserrat_14, COLOR_TEXT_DIM);
    lv_obj_set_pos(lv_obj_get_child(scr, -1), lx, ly);
    ta_hostname = lv_textarea_create(scr);
    lv_textarea_set_one_line(ta_hostname, true);
    lv_textarea_set_max_length(ta_hostname, 31);
    lv_textarea_set_text(ta_hostname, "winder");
    lv_obj_set_size(ta_hostname, 240, 36);
    lv_obj_set_pos(ta_hostname, lx + 80, ly - 4);
    lv_obj_set_style_bg_color(ta_hostname, COLOR_CARD, 0);
    lv_obj_set_style_text_color(ta_hostname, COLOR_TEXT, 0);
    lv_obj_add_event_cb(ta_hostname, cb_ta_focused, LV_EVENT_FOCUSED, nullptr);

    // Right column: notifications
    int rx = 400, ry = 110;

    ui_create_label(scr, "Webhook URL", &lv_font_montserrat_14, COLOR_TEXT_DIM);
    lv_obj_set_pos(lv_obj_get_child(scr, -1), rx, ry);
    ta_webhook = lv_textarea_create(scr);
    lv_textarea_set_one_line(ta_webhook, true);
    lv_textarea_set_max_length(ta_webhook, 127);
    lv_textarea_set_placeholder_text(ta_webhook, "https://...");
    lv_obj_set_size(ta_webhook, 370, 36);
    lv_obj_set_pos(ta_webhook, rx, ry + 20);
    lv_obj_set_style_bg_color(ta_webhook, COLOR_CARD, 0);
    lv_obj_set_style_text_color(ta_webhook, COLOR_TEXT, 0);
    lv_obj_add_event_cb(ta_webhook, cb_ta_focused, LV_EVENT_FOCUSED, nullptr);
    ry += 68;

    ui_create_label(scr, "Notify on complete", &lv_font_montserrat_14, COLOR_TEXT_DIM);
    lv_obj_set_pos(lv_obj_get_child(scr, -1), rx, ry + 4);
    sw_notify_ok = lv_switch_create(scr);
    lv_obj_set_pos(sw_notify_ok, rx + 200, ry);
    lv_obj_add_state(sw_notify_ok, LV_STATE_CHECKED);
    ry += 38;

    ui_create_label(scr, "Notify on error", &lv_font_montserrat_14, COLOR_TEXT_DIM);
    lv_obj_set_pos(lv_obj_get_child(scr, -1), rx, ry + 4);
    sw_notify_err = lv_switch_create(scr);
    lv_obj_set_pos(sw_notify_err, rx + 200, ry);
    lv_obj_add_state(sw_notify_err, LV_STATE_CHECKED);

    // Buttons
    int by = 270;
    ui_create_btn(scr, LV_SYMBOL_WIFI " CONNECT", 150, 44, COLOR_GREEN, cb_connect);
    lv_obj_set_pos(lv_obj_get_child(scr, -1), 20, by);

    ui_create_btn(scr, "DISCONNECT", 140, 44, COLOR_RED, cb_disconnect);
    lv_obj_set_pos(lv_obj_get_child(scr, -1), 180, by);

    ui_create_btn(scr, "START AP", 120, 44, COLOR_ORANGE, cb_start_ap);
    lv_obj_set_pos(lv_obj_get_child(scr, -1), 330, by);

    // Load saved config into fields
    WiFiConfig &wcfg = wifi_get_config();
    lv_textarea_set_text(ta_ssid, wcfg.ssid);
    lv_textarea_set_text(ta_hostname, wcfg.hostname);
    lv_textarea_set_text(ta_webhook, wcfg.notify_url);
    if (!wcfg.notify_on_complete) lv_obj_clear_state(sw_notify_ok, LV_STATE_CHECKED);
    if (!wcfg.notify_on_error) lv_obj_clear_state(sw_notify_err, LV_STATE_CHECKED);

    // Keyboard (hidden by default)
    kb = lv_keyboard_create(scr);
    lv_obj_set_size(kb, 780, 160);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(kb, cb_kb_ready, LV_EVENT_READY, nullptr);
    lv_obj_add_event_cb(kb, cb_kb_ready, LV_EVENT_CANCEL, nullptr);

    return scr;
}

void ui_wifi_update() {
    WiFiState ws = wifi_get_state();
    char buf[64];

    switch (ws) {
        case WiFiState::CONNECTED:
            lv_label_set_text(lbl_wifi_status, "CONNECTED");
            lv_obj_set_style_text_color(lbl_wifi_status, COLOR_GREEN, 0);
            break;
        case WiFiState::CONNECTING:
            lv_label_set_text(lbl_wifi_status, "CONNECTING...");
            lv_obj_set_style_text_color(lbl_wifi_status, COLOR_ORANGE, 0);
            break;
        case WiFiState::AP_ACTIVE:
            lv_label_set_text(lbl_wifi_status, "AP: WinderSetup");
            lv_obj_set_style_text_color(lbl_wifi_status, COLOR_ACCENT, 0);
            break;
        default:
            lv_label_set_text(lbl_wifi_status, "DISCONNECTED");
            lv_obj_set_style_text_color(lbl_wifi_status, COLOR_RED, 0);
            break;
    }

    snprintf(buf, sizeof(buf), "IP: %s", wifi_get_ip());
    lv_label_set_text(lbl_wifi_ip, buf);

    int rssi = wifi_get_rssi();
    if (rssi != 0)
        snprintf(buf, sizeof(buf), "RSSI: %d dBm", rssi);
    else
        snprintf(buf, sizeof(buf), "RSSI: --");
    lv_label_set_text(lbl_wifi_rssi, buf);
}
