#pragma once
#include <lvgl.h>

void display_init();
void display_tick();
void display_backlight_on();
void touch_read(lv_indev_drv_t *drv, lv_indev_data_t *data);
