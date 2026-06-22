// Based on LVGL_CYD v1.2.2 by Rop Gonggrijp <rop@gonggri.jp>
// https://github.com/ropg/LVGL_CYD  (MIT Licence)
// Vendored and modified to support multiple board variants via build-flag
// configuration rather than a single hardcoded board layout.

#ifndef _LVGL_CYD_h_
#define _LVGL_CYD_h_

#define LVGL_CYD_VERSION "1.2.2-multi"

#include <lvgl.h>
#include <TFT_eSPI.h>

#define USB_DOWN  LV_DISPLAY_ROTATION_0
#define USB_RIGHT LV_DISPLAY_ROTATION_90
#define USB_UP    LV_DISPLAY_ROTATION_180
#define USB_LEFT  LV_DISPLAY_ROTATION_270

class LVGL_CYD {
  public:
    static void begin(lv_display_rotation_t rotation = USB_DOWN);
    static void backlight(uint8_t brightness);
    static void led(uint8_t red, uint8_t green, uint8_t blue, bool true_color = true);
    static bool capacitive;
    static bool resistive;
    static int16_t pressure;
    static TFT_eSPI * tft;
};

#endif
