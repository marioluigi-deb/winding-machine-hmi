#include "display.h"
#include "config.h"
#include <Arduino.h>
#include <Wire.h>

#ifdef BOARD_CROWPANEL_P4
// ============================================================
// ESP32-P4: Direct ESP-IDF RGB LCD + Wire-based STC8 backlight
// ============================================================
#include <esp_lcd_panel_rgb.h>
#include <esp_cache.h>
#include <esp_lcd_panel_ops.h>

static esp_lcd_panel_handle_t panel_handle = nullptr;
static uint16_t *p4_framebuffer = nullptr;

// STC8H1KXX backlight MCU via Wire (I2C addr 0x2F)
#define STC8_ADDR       0x2F
#define STC8_REG_GPIO   0x18
#define STC8_REG_PWM    0x20
#define STC8_GPIO_BL    3      // LCD backlight power
#define STC8_PWM_BL     0      // Backlight PWM channel

static void stc8_write_reg(uint8_t reg, uint8_t idx, uint8_t val) {
    Wire.beginTransmission(STC8_ADDR);
    Wire.write(reg + idx);
    Wire.write(val);
    Wire.endTransmission();
}

static void backlight_on() {
    stc8_write_reg(STC8_REG_GPIO, STC8_GPIO_BL, 1);   // power on
    delay(20);
    stc8_write_reg(STC8_REG_PWM, STC8_PWM_BL, 100);   // 100% brightness
}

static void p4_lcd_init() {
    esp_lcd_rgb_panel_config_t panel_cfg = {};

    panel_cfg.clk_src = LCD_CLK_SRC_DEFAULT;
    panel_cfg.timings.pclk_hz = 18 * 1000 * 1000;
    panel_cfg.timings.h_res = SCREEN_W;
    panel_cfg.timings.v_res = SCREEN_H;
    panel_cfg.timings.hsync_pulse_width = 4;
    panel_cfg.timings.hsync_back_porch = 8;
    panel_cfg.timings.hsync_front_porch = 8;
    panel_cfg.timings.vsync_pulse_width = 4;
    panel_cfg.timings.vsync_back_porch = 16;
    panel_cfg.timings.vsync_front_porch = 16;
    panel_cfg.timings.flags.pclk_active_neg = 1;

    panel_cfg.data_width = 16;
    panel_cfg.num_fbs = 1;
    panel_cfg.bounce_buffer_size_px = SCREEN_W * 10;
    panel_cfg.psram_trans_align = 64;
    panel_cfg.sram_trans_align = 4;
    panel_cfg.flags.fb_in_psram = 1;

    panel_cfg.hsync_gpio_num = 40;
    panel_cfg.vsync_gpio_num = 41;
    panel_cfg.de_gpio_num = 2;
    panel_cfg.pclk_gpio_num = 3;
    panel_cfg.disp_gpio_num = -1;

    panel_cfg.data_gpio_nums[0]  = 8;
    panel_cfg.data_gpio_nums[1]  = 7;
    panel_cfg.data_gpio_nums[2]  = 6;
    panel_cfg.data_gpio_nums[3]  = 5;
    panel_cfg.data_gpio_nums[4]  = 4;
    panel_cfg.data_gpio_nums[5]  = 14;
    panel_cfg.data_gpio_nums[6]  = 13;
    panel_cfg.data_gpio_nums[7]  = 12;
    panel_cfg.data_gpio_nums[8]  = 11;
    panel_cfg.data_gpio_nums[9]  = 10;
    panel_cfg.data_gpio_nums[10] = 9;
    panel_cfg.data_gpio_nums[11] = 19;
    panel_cfg.data_gpio_nums[12] = 18;
    panel_cfg.data_gpio_nums[13] = 17;
    panel_cfg.data_gpio_nums[14] = 16;
    panel_cfg.data_gpio_nums[15] = 15;

    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_cfg, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

    esp_lcd_rgb_panel_get_frame_buffer(panel_handle, 1, (void **)&p4_framebuffer);
}

static void p4_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
    if (p4_framebuffer) {
        int w = area->x2 - area->x1 + 1;
        uint16_t *src = (uint16_t *)color_p;
        for (int y = area->y1; y <= area->y2; y++) {
            memcpy(&p4_framebuffer[y * SCREEN_W + area->x1], src, w * 2);
            src += w;
        }
    }
    lv_disp_flush_ready(drv);
}

static bool touch_pressed = false;
static uint16_t touch_x = 0, touch_y = 0;

static void gt911_reset_p4() {
    pinMode(42, OUTPUT);
    pinMode(36, OUTPUT);
    digitalWrite(42, LOW);
    digitalWrite(36, LOW);
    delay(10);
    digitalWrite(42, LOW);
    delay(1);
    digitalWrite(36, HIGH);
    delay(5);
    pinMode(42, INPUT);
    delay(50);
}

static bool gt911_read_p4() {
    Wire.beginTransmission(0x5D);
    Wire.write(0x81); Wire.write(0x4E);
    if (Wire.endTransmission() != 0) return false;
    Wire.requestFrom((uint8_t)0x5D, (uint8_t)1);
    if (!Wire.available()) return false;
    uint8_t status = Wire.read();
    bool touched = (status & 0x80) && ((status & 0x0F) > 0);
    if (touched) {
        Wire.beginTransmission(0x5D);
        Wire.write(0x81); Wire.write(0x50);
        Wire.endTransmission();
        Wire.requestFrom((uint8_t)0x5D, (uint8_t)4);
        if (Wire.available() >= 4) {
            uint8_t xl = Wire.read(), xh = Wire.read();
            uint8_t yl = Wire.read(), yh = Wire.read();
            touch_x = (xh << 8) | xl;
            touch_y = (yh << 8) | yl;
            touch_pressed = true;
        }
    } else {
        touch_pressed = false;
    }
    Wire.beginTransmission(0x5D);
    Wire.write(0x81); Wire.write(0x4E); Wire.write(0x00);
    Wire.endTransmission();
    return touched;
}

static void p4_touch_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    gt911_read_p4();
    data->point.x = touch_x;
    data->point.y = touch_y;
    data->state = touch_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

void display_init() {
    Serial.println("INIT: Wire I2C on SDA=45 SCL=46");
    Wire.begin(45, 46);
    Wire.setClock(400000);
    Wire.setTimeout(50);

    // Backlight stays OFF until display_backlight_on() is called after WiFi init

    Serial.println("INIT: GT911 touch reset");
    gt911_reset_p4();

    Serial.println("INIT: RGB LCD panel");
    Serial.printf("INIT: free heap before LCD: %d\n", esp_get_free_heap_size());
    p4_lcd_init();
    Serial.printf("INIT: free heap after LCD: %d\n", esp_get_free_heap_size());

    Serial.println("INIT: lv_init");
    lv_init();

    Serial.println("INIT: alloc buffers");
    static lv_disp_draw_buf_t disp_buf;
    size_t buf_size = SCREEN_W * 10;
    lv_color_t *buf1 = (lv_color_t *)malloc(buf_size * sizeof(lv_color_t));
    Serial.printf("INIT: buf=%p size=%d bytes\n", buf1, buf_size * sizeof(lv_color_t));
    lv_disp_draw_buf_init(&disp_buf, buf1, nullptr, buf_size);
    Serial.flush();
    Serial.println("INIT: draw_buf_init done");

    Serial.flush();
    Serial.println("INIT: disp_drv_init");
    Serial.flush();
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCREEN_W;
    disp_drv.ver_res = SCREEN_H;
    disp_drv.flush_cb = p4_flush_cb;
    disp_drv.draw_buf = &disp_buf;
    Serial.println("INIT: disp_drv_register...");
    Serial.flush();
    lv_disp_drv_register(&disp_drv);
    Serial.println("INIT: disp registered OK");

    Serial.println("INIT: touch driver");
    Serial.flush();
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = p4_touch_read;
    lv_indev_drv_register(&indev_drv);
    Serial.println("INIT: touch registered OK");

    Serial.println("INIT: theme");
    lv_theme_t *th = lv_theme_default_init(
        lv_disp_get_default(),
        lv_palette_main(LV_PALETTE_BLUE),
        lv_palette_main(LV_PALETTE_CYAN),
        true, LV_FONT_DEFAULT
    );
    lv_disp_set_theme(lv_disp_get_default(), th);

    Serial.println("INIT: display complete!");
}

void display_backlight_on() {
    Serial.println("BACKLIGHT: on");
    lv_obj_invalidate(lv_scr_act());
    lv_timer_handler();  // force one full redraw
    delay(50);
    backlight_on();
}

#else
// ============================================================
// ESP32-S3: RGB parallel LCD via Arduino_GFX
// ============================================================
#include <Arduino_GFX_Library.h>

static Arduino_ESP32RGBPanel *rgb_panel = new Arduino_ESP32RGBPanel(
    PIN_LCD_DE, PIN_LCD_VSYNC, PIN_LCD_HSYNC, PIN_LCD_PCLK,
    PIN_LCD_R0, PIN_LCD_R1, PIN_LCD_R2, PIN_LCD_R3, PIN_LCD_R4,
    PIN_LCD_G0, PIN_LCD_G1, PIN_LCD_G2, PIN_LCD_G3, PIN_LCD_G4, PIN_LCD_G5,
    PIN_LCD_B0, PIN_LCD_B1, PIN_LCD_B2, PIN_LCD_B3, PIN_LCD_B4,
    1, 8, 4, 43, 1, 8, 4, 12, 1, 16000000
);

static Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
    SCREEN_W, SCREEN_H, rgb_panel, 0, true
);

static bool s3_touch_pressed = false;
static uint16_t s3_touch_x = 0, s3_touch_y = 0;

static void gt911_reset_s3() {
    pinMode(PIN_TOUCH_INT, OUTPUT);
    pinMode(PIN_TOUCH_RST, OUTPUT);
    digitalWrite(PIN_TOUCH_INT, LOW);
    digitalWrite(PIN_TOUCH_RST, LOW);
    delay(10);
    digitalWrite(PIN_TOUCH_INT, LOW);
    delay(1);
    digitalWrite(PIN_TOUCH_RST, HIGH);
    delay(5);
    pinMode(PIN_TOUCH_INT, INPUT);
    delay(50);
}

static bool gt911_read_s3() {
    Wire.beginTransmission(TOUCH_I2C_ADDR);
    Wire.write(0x81); Wire.write(0x4E);
    if (Wire.endTransmission() != 0) return false;
    Wire.requestFrom((uint8_t)TOUCH_I2C_ADDR, (uint8_t)1);
    if (!Wire.available()) return false;
    uint8_t status = Wire.read();
    bool touched = (status & 0x80) && ((status & 0x0F) > 0);
    if (touched) {
        Wire.beginTransmission(TOUCH_I2C_ADDR);
        Wire.write(0x81); Wire.write(0x50);
        Wire.endTransmission();
        Wire.requestFrom((uint8_t)TOUCH_I2C_ADDR, (uint8_t)4);
        if (Wire.available() >= 4) {
            uint8_t xl = Wire.read(), xh = Wire.read();
            uint8_t yl = Wire.read(), yh = Wire.read();
            s3_touch_x = (xh << 8) | xl;
            s3_touch_y = (yh << 8) | yl;
            s3_touch_pressed = true;
        }
    } else {
        s3_touch_pressed = false;
    }
    Wire.beginTransmission(TOUCH_I2C_ADDR);
    Wire.write(0x81); Wire.write(0x4E); Wire.write(0x00);
    Wire.endTransmission();
    return touched;
}

static void s3_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, area->y2 - area->y1 + 1);
    lv_disp_flush_ready(drv);
}

static void s3_touch_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    gt911_read_s3();
    data->point.x = s3_touch_x;
    data->point.y = s3_touch_y;
    data->state = s3_touch_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

void display_init() {
    pinMode(PIN_LCD_BL, OUTPUT);
    digitalWrite(PIN_LCD_BL, HIGH);
    gfx->begin();
    gfx->fillScreen(BLACK);

    Wire.begin(PIN_TOUCH_SDA, PIN_TOUCH_SCL);
    Wire.setClock(400000);
    gt911_reset_s3();

    lv_init();

    static lv_disp_draw_buf_t draw_buf;
    size_t buf_size = SCREEN_W * 80;
    lv_color_t *buf1 = (lv_color_t *)heap_caps_malloc(buf_size * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    lv_color_t *buf2 = (lv_color_t *)heap_caps_malloc(buf_size * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, buf_size);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCREEN_W;
    disp_drv.ver_res = SCREEN_H;
    disp_drv.flush_cb = s3_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = s3_touch_read;
    lv_indev_drv_register(&indev_drv);

    lv_theme_t *th = lv_theme_default_init(
        lv_disp_get_default(),
        lv_palette_main(LV_PALETTE_BLUE),
        lv_palette_main(LV_PALETTE_CYAN),
        true, LV_FONT_DEFAULT
    );
    lv_disp_set_theme(lv_disp_get_default(), th);
}

#endif

void display_tick() {
    lv_timer_handler();
}
