#include "web_server.h"
#include "config.h"
#include "motion.h"
#include "storage.h"
#include "wifi_manager.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <HTTPUpdateServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

static WebServer server(80);
static HTTPUpdateServer ota;
static WiFiClient sse_client;
static unsigned long last_sse = 0;

static const char *status_str(MachineStatus s) {
    switch (s) {
        case MachineStatus::DISCONNECTED: return "DISCONNECTED";
        case MachineStatus::IDLE:         return "IDLE";
        case MachineStatus::HOMING:       return "HOMING";
        case MachineStatus::MOVING:       return "MOVING";
        case MachineStatus::WINDING:      return "WINDING";
        case MachineStatus::PAUSED:       return "PAUSED";
        case MachineStatus::ESTOP:        return "ESTOP";
        case MachineStatus::ERROR:        return "ERROR";
    }
    return "UNKNOWN";
}

static void build_status_json(char *buf, size_t len) {
    const MachineState &ms = motion_get_state();
    snprintf(buf, len,
        "{\"status\":\"%s\",\"turn\":%d,\"layer\":%d,"
        "\"x\":%.3f,\"y\":%.3f,\"rpm\":%.1f,"
        "\"hx\":%d,\"hy\":%d,\"rssi\":%d,\"ip\":\"%s\"}",
        status_str(ms.status), ms.current_turn, ms.current_layer,
        ms.pos_x, ms.pos_y, ms.spindle_rpm,
        ms.homed_x, ms.homed_y,
        wifi_get_rssi(), wifi_get_ip());
}

// --- Route handlers ---

static void handle_api_status() {
    char json[300];
    build_status_json(json, sizeof(json));
    server.send(200, "application/json", json);
}

static void handle_api_events() {
    sse_client = server.client();
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/event-stream", "");
    server.sendContent("retry: 1000\n\n");
}

static void handle_api_programs_list() {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    for (int i = 0; i < MAX_PROGRAMS; i++) {
        WindingProgram prog;
        if (storage_load_program(i, prog)) {
            JsonObject obj = arr.add<JsonObject>();
            obj["slot"] = i;
            obj["name"] = prog.name;
            obj["turns"] = prog.total_turns;
            obj["layers"] = prog.num_layers;
            obj["awg"] = prog.awg;
            obj["speed"] = prog.speed_rpm;
        }
    }
    String out;
    serializeJson(doc, out);
    server.send(200, "application/json", out);
}

static void handle_api_program_get() {
    int slot = server.arg("slot").toInt();
    WindingProgram prog;
    if (!storage_load_program(slot, prog)) {
        server.send(404, "application/json", "{\"error\":\"not found\"}");
        return;
    }
    JsonDocument doc;
    doc["slot"] = slot;
    doc["name"] = prog.name;
    doc["awg"] = prog.awg;
    doc["wire_od"] = prog.wire_od_mm;
    doc["turns"] = prog.total_turns;
    doc["layers"] = prog.num_layers;
    doc["bobbin_width"] = prog.bobbin_width_mm;
    doc["bobbin_id"] = prog.bobbin_id_mm;
    doc["speed"] = prog.speed_rpm;
    doc["direction_cw"] = prog.direction_cw;
    doc["layer_step"] = prog.layer_step_mm;
    String out;
    serializeJson(doc, out);
    server.send(200, "application/json", out);
}

static void handle_api_program_save() {
    int slot = server.arg("slot").toInt();
    if (slot < 0 || slot >= MAX_PROGRAMS) {
        server.send(400, "application/json", "{\"error\":\"bad slot\"}");
        return;
    }
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, server.arg("plain"));
    if (err) {
        server.send(400, "application/json", "{\"error\":\"bad json\"}");
        return;
    }
    WindingProgram prog = {};
    strlcpy(prog.name, doc["name"] | "Untitled", sizeof(prog.name));
    prog.awg             = doc["awg"] | 24;
    prog.wire_od_mm      = doc["wire_od"] | 0.52f;
    prog.total_turns     = doc["turns"] | 100;
    prog.num_layers      = doc["layers"] | 1;
    prog.bobbin_width_mm = doc["bobbin_width"] | 50.0f;
    prog.bobbin_id_mm    = doc["bobbin_id"] | 25.0f;
    prog.speed_rpm       = doc["speed"] | 200.0f;
    prog.direction_cw    = doc["direction_cw"] | true;
    prog.layer_step_mm   = doc["layer_step"] | 0.0f;
    storage_save_program(slot, prog);
    server.send(200, "application/json", "{\"ok\":true}");
}

static void handle_api_program_delete() {
    int slot = server.arg("slot").toInt();
    storage_delete_program(slot);
    server.send(200, "application/json", "{\"ok\":true}");
}

static void handle_control_start()  { motion_wind_start(); server.send(200, "application/json", "{\"ok\":true}"); }
static void handle_control_pause()  { motion_wind_pause(); server.send(200, "application/json", "{\"ok\":true}"); }
static void handle_control_resume() { motion_wind_resume(); server.send(200, "application/json", "{\"ok\":true}"); }
static void handle_control_abort()  { motion_wind_abort(); server.send(200, "application/json", "{\"ok\":true}"); }
static void handle_control_estop()  { motion_estop(); server.send(200, "application/json", "{\"ok\":true}"); }
static void handle_control_reset()  { motion_reset(); server.send(200, "application/json", "{\"ok\":true}"); }

static void handle_control_speed() {
    float rpm = server.arg("rpm").toFloat();
    motion_wind_speed(rpm);
    server.send(200, "application/json", "{\"ok\":true}");
}

static void handle_wifi_config_get() {
    WiFiConfig &cfg = wifi_get_config();
    JsonDocument doc;
    doc["ssid"] = cfg.ssid;
    doc["hostname"] = cfg.hostname;
    doc["notify_url"] = cfg.notify_url;
    doc["auto_connect"] = cfg.auto_connect;
    doc["notify_on_complete"] = cfg.notify_on_complete;
    doc["notify_on_error"] = cfg.notify_on_error;
    String out;
    serializeJson(doc, out);
    server.send(200, "application/json", out);
}

static void handle_wifi_config_save() {
    JsonDocument doc;
    deserializeJson(doc, server.arg("plain"));
    WiFiConfig &cfg = wifi_get_config();
    strlcpy(cfg.ssid, doc["ssid"] | cfg.ssid, sizeof(cfg.ssid));
    if (doc["password"].is<const char*>())
        strlcpy(cfg.password, doc["password"], sizeof(cfg.password));
    strlcpy(cfg.hostname, doc["hostname"] | cfg.hostname, sizeof(cfg.hostname));
    strlcpy(cfg.notify_url, doc["notify_url"] | cfg.notify_url, sizeof(cfg.notify_url));
    cfg.auto_connect       = doc["auto_connect"] | cfg.auto_connect;
    cfg.notify_on_complete = doc["notify_on_complete"] | cfg.notify_on_complete;
    cfg.notify_on_error    = doc["notify_on_error"] | cfg.notify_on_error;
    storage_save_wifi(cfg);
    server.send(200, "application/json", "{\"ok\":true,\"msg\":\"saved, reconnecting\"}");
    delay(500);
    wifi_connect(cfg.ssid, cfg.password);
}

void web_server_init() {
    LittleFS.begin(true);

    // API routes
    server.on("/api/status", HTTP_GET, handle_api_status);
    server.on("/api/events", HTTP_GET, handle_api_events);
    server.on("/api/programs", HTTP_GET, handle_api_programs_list);
    server.on("/api/program", HTTP_GET, handle_api_program_get);
    server.on("/api/program", HTTP_POST, handle_api_program_save);
    server.on("/api/program", HTTP_DELETE, handle_api_program_delete);
    server.on("/api/control/start", HTTP_POST, handle_control_start);
    server.on("/api/control/pause", HTTP_POST, handle_control_pause);
    server.on("/api/control/resume", HTTP_POST, handle_control_resume);
    server.on("/api/control/abort", HTTP_POST, handle_control_abort);
    server.on("/api/control/estop", HTTP_POST, handle_control_estop);
    server.on("/api/control/reset", HTTP_POST, handle_control_reset);
    server.on("/api/control/speed", HTTP_POST, handle_control_speed);
    server.on("/api/wifi", HTTP_GET, handle_wifi_config_get);
    server.on("/api/wifi", HTTP_POST, handle_wifi_config_save);

    // OTA update at /update
    ota.setup(&server, "/update", "admin", "winder");

    // Serve dashboard from LittleFS, fallback to embedded
    server.serveStatic("/", LittleFS, "/", "max-age=86400");

    // Fallback: serve embedded dashboard if LittleFS is empty
    server.onNotFound([]() {
        if (server.uri() == "/" || server.uri() == "/index.html") {
            server.send(200, "text/html",
                "<!DOCTYPE html><html><head><meta charset=utf-8><meta name=viewport content='width=device-width,initial-scale=1'>"
                "<title>Winder</title><style>"
                "*{margin:0;padding:0;box-sizing:border-box}"
                "body{font-family:system-ui;background:#1a1a2e;color:#eaeaea;padding:16px}"
                ".card{background:#16213e;border-radius:10px;padding:16px;margin-bottom:12px}"
                "h1{color:#00b4d8;font-size:1.4em;margin-bottom:8px}"
                "h2{color:#8899aa;font-size:.9em;font-weight:normal;margin-bottom:12px}"
                ".big{font-size:3em;color:#06d6a0;font-weight:bold}"
                ".row{display:flex;gap:8px;flex-wrap:wrap}"
                ".stat{flex:1;min-width:120px}"
                ".stat label{color:#8899aa;font-size:.8em;display:block}"
                ".stat span{font-size:1.3em}"
                "btn,button{background:#0f3460;color:white;border:none;padding:12px 20px;border-radius:8px;font-size:1em;cursor:pointer;margin:4px}"
                ".green{background:#06d6a0;color:#1a1a2e}.red{background:#e63946}.orange{background:#f77f00}"
                "#status{display:inline-block;padding:4px 12px;border-radius:4px;font-weight:bold}"
                ".bar{background:#0f3460;border-radius:6px;height:24px;margin:8px 0}"
                ".bar-fill{background:#06d6a0;height:100%;border-radius:6px;transition:width .3s}"
                "</style></head><body>"
                "<h1>WINDING MACHINE</h1>"
                "<h2>Remote Dashboard</h2>"
                "<div class=card><div id=status>CONNECTING...</div></div>"
                "<div class=card><div class=big id=turns>0</div><div id=turns-total>/ 0 turns</div>"
                "<div class=bar><div class=bar-fill id=prog style=width:0%></div></div>"
                "<div class=row>"
                "<div class=stat><label>Layer</label><span id=layer>0</span></div>"
                "<div class=stat><label>Speed</label><span id=rpm>0</span> RPM</div>"
                "<div class=stat><label>X pos</label><span id=posx>0.000</span></div>"
                "<div class=stat><label>Y pos</label><span id=posy>0.000</span></div>"
                "</div></div>"
                "<div class=card><div class=row>"
                "<button class=green onclick=cmd('start')>START</button>"
                "<button class=orange onclick=cmd('pause')>PAUSE</button>"
                "<button onclick=cmd('resume')>RESUME</button>"
                "<button class=red onclick=cmd('abort')>ABORT</button>"
                "<button class=red onclick=cmd('estop')>E-STOP</button>"
                "</div></div>"
                "<div class=card><a href=/update style=color:#00b4d8>Firmware Update</a></div>"
                "<script>"
                "const $=id=>document.getElementById(id);"
                "function cmd(c){fetch('/api/control/'+c,{method:'POST'})}"
                "const src=new EventSource('/api/events');"
                "src.onmessage=e=>{"
                "const d=JSON.parse(e.data);"
                "$('status').textContent=d.status;"
                "$('status').style.background=d.status=='WINDING'?'#06d6a0':d.status=='PAUSED'?'#f77f00':d.status=='ERROR'||d.status=='ESTOP'?'#e63946':'#0f3460';"
                "$('status').style.color=d.status=='WINDING'?'#1a1a2e':'white';"
                "$('turns').textContent=d.turn;"
                "$('layer').textContent=d.layer;"
                "$('rpm').textContent=d.rpm.toFixed(0);"
                "$('posx').textContent=d.x.toFixed(3);"
                "$('posy').textContent=d.y.toFixed(3);"
                "};"
                "src.onerror=()=>$('status').textContent='OFFLINE';"
                "</script></body></html>"
            );
        } else {
            server.send(404, "text/plain", "Not found");
        }
    });

    server.begin();
    Serial.printf("WEB: server started on port 80\n");
}

void web_server_poll() {
    server.handleClient();

    // SSE push at ~5Hz
    if (sse_client.connected() && millis() - last_sse > 200) {
        last_sse = millis();
        char json[300];
        build_status_json(json, sizeof(json));
        sse_client.printf("data: %s\n\n", json);
    } else if (!sse_client.connected()) {
        sse_client.stop();
    }
}
