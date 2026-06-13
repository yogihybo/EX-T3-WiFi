#include "LVGL_Layouts.h"

SemaphoreHandle_t lvgl_mutex = NULL;

lv_obj_t* header_bar;
lv_obj_t* main_tabview;

lv_obj_t* loco_tab;
lv_obj_t* acc_tab;
lv_obj_t* pwr_tab;
lv_obj_t* set_tab;

static lv_obj_t* wifi_label;
static lv_obj_t* cs_label;
static lv_obj_t* power_label;
static lv_obj_t* loco_label;

void setup_lvgl_layouts() {
    lvgl_mutex = xSemaphoreCreateMutex();

    lv_obj_t* scr = lv_scr_act();
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_set_style_pad_row(scr, 0, 0);
    lv_obj_set_style_pad_column(scr, 0, 0);

    // Top Header (30px height)
    header_bar = lv_obj_create(scr);
    lv_obj_set_size(header_bar, LV_PCT(100), 30);
    lv_obj_set_style_pad_all(header_bar, 0, 0);
    lv_obj_set_style_border_width(header_bar, 0, 0);
    lv_obj_set_flex_flow(header_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header_bar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(header_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_pad_column(header_bar, 15, 0);

    power_label = lv_label_create(header_bar);
    lv_label_set_text(power_label, LV_SYMBOL_BATTERY_FULL " --");

    wifi_label = lv_label_create(header_bar);
    lv_label_set_text(wifi_label, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(wifi_label, lv_color_make(255, 0, 0), 0);

    cs_label = lv_label_create(header_bar);
    lv_label_set_text(cs_label, LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_color(cs_label, lv_color_make(255, 0, 0), 0);

    loco_label = lv_label_create(header_bar);
    lv_label_set_text(loco_label, LV_SYMBOL_LIST " 0");

    // Main Tabview (middle to bottom)
    main_tabview = lv_tabview_create(scr);
    lv_tabview_set_tab_bar_position(main_tabview, LV_DIR_BOTTOM);
    lv_tabview_set_tab_bar_size(main_tabview, 40);
    lv_obj_set_width(main_tabview, LV_PCT(100));
    lv_obj_set_flex_grow(main_tabview, 1); // Fills remaining vertical space
    
    // Remove padding to allow views to fill it seamlessly
    lv_obj_t* tab_btns = lv_tabview_get_tab_bar(main_tabview);
    lv_obj_set_style_pad_all(tab_btns, 0, 0);

    loco_tab = lv_tabview_add_tab(main_tabview, "Loco");
    acc_tab = lv_tabview_add_tab(main_tabview, "Acc");
    pwr_tab = lv_tabview_add_tab(main_tabview, "Pwr");
    set_tab = lv_tabview_add_tab(main_tabview, "Set");

    lv_obj_set_style_pad_all(loco_tab, 0, 0);
    lv_obj_set_style_pad_all(acc_tab, 0, 0);
    lv_obj_set_style_pad_all(pwr_tab, 0, 0);
    lv_obj_set_style_pad_all(set_tab, 0, 0);
}

void set_header_loco_count(int count) {
    if (loco_label) lv_label_set_text_fmt(loco_label, LV_SYMBOL_LIST " %d", count);
}

void set_header_wifi_status(bool connected) {
    if (wifi_label) {
        lv_label_set_text(wifi_label, LV_SYMBOL_WIFI);
        lv_obj_set_style_text_color(wifi_label, connected ? lv_color_make(0, 255, 0) : lv_color_make(255, 0, 0), 0);
    }
}

void set_header_cs_status(bool connected) {
    if (cs_label) {
        lv_label_set_text(cs_label, LV_SYMBOL_SETTINGS);
        lv_obj_set_style_text_color(cs_label, connected ? lv_color_make(0, 255, 0) : lv_color_make(255, 0, 0), 0);
    }
}

void set_header_power_status(float voltage) {
    if (power_label) lv_label_set_text_fmt(power_label, LV_SYMBOL_BATTERY_FULL " %.2fV", voltage);
}
