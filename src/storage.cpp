#include "storage.h"
#include <Preferences.h>
#include <string.h>

static Preferences prefs;

void storage_init() {
    prefs.begin("winder", false);
}

void storage_load_config(MachineConfig &cfg) {
    cfg.steps_per_mm_x  = prefs.getFloat("spm_x", DEFAULT_STEPS_PER_MM_X);
    cfg.steps_per_mm_y  = prefs.getFloat("spm_y", DEFAULT_STEPS_PER_MM_Y);
    cfg.steps_per_mm_z  = prefs.getFloat("spm_z", DEFAULT_STEPS_PER_MM_Z);
    cfg.steps_per_rev   = prefs.getFloat("spr",   DEFAULT_STEPS_PER_REV);
    cfg.max_feed_x      = prefs.getFloat("mf_x",  DEFAULT_MAX_FEED_X);
    cfg.max_feed_y      = prefs.getFloat("mf_y",  DEFAULT_MAX_FEED_Y);
    cfg.max_feed_z      = prefs.getFloat("mf_z",  DEFAULT_MAX_FEED_Z);
    cfg.max_spindle_rpm = prefs.getFloat("msr",   DEFAULT_MAX_SPINDLE_RPM);
    cfg.accel_x         = prefs.getFloat("acc_x", DEFAULT_ACCEL_X);
    cfg.accel_y         = prefs.getFloat("acc_y", DEFAULT_ACCEL_Y);
    cfg.accel_z         = prefs.getFloat("acc_z", DEFAULT_ACCEL_Z);
}

void storage_save_config(const MachineConfig &cfg) {
    prefs.putFloat("spm_x", cfg.steps_per_mm_x);
    prefs.putFloat("spm_y", cfg.steps_per_mm_y);
    prefs.putFloat("spm_z", cfg.steps_per_mm_z);
    prefs.putFloat("spr",   cfg.steps_per_rev);
    prefs.putFloat("mf_x",  cfg.max_feed_x);
    prefs.putFloat("mf_y",  cfg.max_feed_y);
    prefs.putFloat("mf_z",  cfg.max_feed_z);
    prefs.putFloat("msr",   cfg.max_spindle_rpm);
    prefs.putFloat("acc_x", cfg.accel_x);
    prefs.putFloat("acc_y", cfg.accel_y);
    prefs.putFloat("acc_z", cfg.accel_z);
}

static void slot_key(int slot, const char *suffix, char *out) {
    snprintf(out, 16, "p%d_%s", slot, suffix);
}

int storage_program_count() {
    int count = 0;
    for (int i = 0; i < MAX_PROGRAMS; i++) {
        char key[16];
        slot_key(i, "name", key);
        if (prefs.isKey(key)) count++;
    }
    return count;
}

bool storage_load_program(int slot, WindingProgram &prog) {
    if (slot < 0 || slot >= MAX_PROGRAMS) return false;
    char key[16];
    slot_key(slot, "name", key);
    if (!prefs.isKey(key)) return false;

    prefs.getString(key, prog.name, sizeof(prog.name));
    slot_key(slot, "wod", key);  prog.wire_od_mm      = prefs.getFloat(key, 0.5f);
    slot_key(slot, "awg", key);  prog.awg             = prefs.getInt(key, 24);
    slot_key(slot, "bw",  key);  prog.bobbin_width_mm = prefs.getFloat(key, 50.0f);
    slot_key(slot, "bid", key);  prog.bobbin_id_mm    = prefs.getFloat(key, 20.0f);
    slot_key(slot, "tt",  key);  prog.total_turns     = prefs.getInt(key, 100);
    slot_key(slot, "nl",  key);  prog.num_layers      = prefs.getInt(key, 1);
    slot_key(slot, "spd", key);  prog.speed_rpm       = prefs.getFloat(key, 200.0f);
    slot_key(slot, "ts",  key);  prog.traverse_start_mm = prefs.getFloat(key, 0.0f);
    slot_key(slot, "dcw", key);  prog.direction_cw    = prefs.getBool(key, true);
    slot_key(slot, "ls",  key);  prog.layer_step_mm   = prefs.getFloat(key, 0.0f);
    return true;
}

bool storage_save_program(int slot, const WindingProgram &prog) {
    if (slot < 0 || slot >= MAX_PROGRAMS) return false;
    char key[16];
    slot_key(slot, "name", key); prefs.putString(key, prog.name);
    slot_key(slot, "wod", key);  prefs.putFloat(key, prog.wire_od_mm);
    slot_key(slot, "awg", key);  prefs.putInt(key, prog.awg);
    slot_key(slot, "bw",  key);  prefs.putFloat(key, prog.bobbin_width_mm);
    slot_key(slot, "bid", key);  prefs.putFloat(key, prog.bobbin_id_mm);
    slot_key(slot, "tt",  key);  prefs.putInt(key, prog.total_turns);
    slot_key(slot, "nl",  key);  prefs.putInt(key, prog.num_layers);
    slot_key(slot, "spd", key);  prefs.putFloat(key, prog.speed_rpm);
    slot_key(slot, "ts",  key);  prefs.putFloat(key, prog.traverse_start_mm);
    slot_key(slot, "dcw", key);  prefs.putBool(key, prog.direction_cw);
    slot_key(slot, "ls",  key);  prefs.putFloat(key, prog.layer_step_mm);
    return true;
}

bool storage_delete_program(int slot) {
    if (slot < 0 || slot >= MAX_PROGRAMS) return false;
    char key[16];
    const char *suffixes[] = {"name","wod","awg","bw","bid","tt","nl","spd","ts","dcw","ls"};
    for (auto s : suffixes) {
        slot_key(slot, s, key);
        prefs.remove(key);
    }
    return true;
}

void storage_get_program_names(char names[][32], int &count) {
    count = 0;
    for (int i = 0; i < MAX_PROGRAMS; i++) {
        char key[16];
        slot_key(i, "name", key);
        if (prefs.isKey(key)) {
            prefs.getString(key, names[count], 32);
            count++;
        } else {
            snprintf(names[count], 32, "(empty slot %d)", i);
            count++;
        }
    }
}

// WiFi config — separate namespace
static Preferences wifi_prefs;
static bool wifi_prefs_init = false;

static void ensure_wifi_prefs() {
    if (!wifi_prefs_init) {
        wifi_prefs.begin("wifi_cfg", false);
        wifi_prefs_init = true;
    }
}

void storage_load_wifi(WiFiConfig &cfg) {
    ensure_wifi_prefs();
    wifi_prefs.getString("ssid", cfg.ssid, sizeof(cfg.ssid));
    wifi_prefs.getString("pass", cfg.password, sizeof(cfg.password));
    wifi_prefs.getString("host", cfg.hostname, sizeof(cfg.hostname));
    wifi_prefs.getString("nurl", cfg.notify_url, sizeof(cfg.notify_url));
    cfg.auto_connect       = wifi_prefs.getBool("auto", true);
    cfg.notify_on_complete = wifi_prefs.getBool("n_ok", true);
    cfg.notify_on_error    = wifi_prefs.getBool("n_err", true);
    if (cfg.hostname[0] == '\0') strncpy(cfg.hostname, "winder", sizeof(cfg.hostname));
}

void storage_save_wifi(const WiFiConfig &cfg) {
    ensure_wifi_prefs();
    wifi_prefs.putString("ssid", cfg.ssid);
    wifi_prefs.putString("pass", cfg.password);
    wifi_prefs.putString("host", cfg.hostname);
    wifi_prefs.putString("nurl", cfg.notify_url);
    wifi_prefs.putBool("auto", cfg.auto_connect);
    wifi_prefs.putBool("n_ok", cfg.notify_on_complete);
    wifi_prefs.putBool("n_err", cfg.notify_on_error);
}
