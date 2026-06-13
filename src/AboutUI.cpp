#include "AboutUI.h"
#include <Version.h>
#include <WiFi.h>

AboutUI::AboutUI(DCCExCS& dccExCS, lv_obj_t* parent) : _dccExCS(dccExCS) {
  _container = lv_obj_create(parent);
  lv_obj_set_size(_container, LV_PCT(100), LV_PCT(100));
  lv_obj_align(_container, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_pad_all(_container, 0, 0);
  lv_obj_set_style_border_width(_container, 0, 0);
  lv_obj_set_style_bg_color(_container, lv_color_make(30, 30, 30), 0);
  lv_obj_set_style_bg_opa(_container, LV_OPA_COVER, 0);
  lv_obj_set_flex_flow(_container, LV_FLEX_FLOW_COLUMN);

  // Title Row
  lv_obj_t* title_row = lv_obj_create(_container);
  lv_obj_set_width(title_row, LV_PCT(100));
  lv_obj_set_height(title_row, 40);
  lv_obj_set_style_pad_all(title_row, 0, 0);
  lv_obj_set_style_border_width(title_row, 0, 0);
  lv_obj_clear_flag(title_row, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* title = lv_label_create(title_row);
  lv_label_set_text(title, "About Controller");
  lv_obj_align(title, LV_ALIGN_LEFT_MID, 10, 0);

  lv_obj_t* close_btn = lv_btn_create(title_row);
  lv_obj_align(close_btn, LV_ALIGN_RIGHT_MID, -10, 0);
  lv_obj_set_style_bg_color(close_btn, lv_color_make(200, 50, 50), 0);
  lv_obj_t* close_lbl = lv_label_create(close_btn);
  lv_label_set_text(close_lbl, "Back");
  lv_obj_center(close_lbl);
  lv_obj_add_event_cb(close_btn, close_btn_event_cb, LV_EVENT_CLICKED, this);

  // Content
  String version("T3-EX-WiFi Version: ");
  version += T3_VERSION_MAJOR; version += "."; version += T3_VERSION_MINOR; version += "."; version += T3_VERSION_PATCH;
  lv_obj_t* t3_ver = lv_label_create(_container);
  lv_label_set_text(t3_ver, version.c_str());

  // WiFi Status
  lv_obj_t* wifi_stat = lv_label_create(_container);
  if (WiFi.status() == WL_CONNECTED) {
      lv_label_set_text_fmt(wifi_stat, "WiFi: Connected | IP: %s", WiFi.localIP().toString().c_str());
  } else {
      lv_label_set_text(wifi_stat, "WiFi: Disconnected");
  }

  // DCC EX Section
  lv_obj_t* cs_title = lv_label_create(_container);
  lv_label_set_text(cs_title, "About DCC-EX Command Station");
  lv_obj_set_style_text_color(cs_title, lv_color_make(52, 204, 211), 0); // Cyan color
  lv_obj_set_style_pad_top(cs_title, 10, 0);

  _csVersion = lv_label_create(_container);
  lv_label_set_text(_csVersion, "Version: Loading...");
  _csBoard = lv_label_create(_container);
  lv_label_set_text(_csBoard, "Board: Loading...");
  _csShield = lv_label_create(_container);
  lv_label_set_text(_csShield, "Shield: Loading...");
  _csBuild = lv_label_create(_container);
  lv_label_set_text(_csBuild, "Build: Loading...");

  _csVersionHandler = _dccExCS.addEventListener(DCCExCS::Event::VERSION, [this](void* parameter) {
    if (xSemaphoreTake(lvgl_mutex, portMAX_DELAY) == pdTRUE) {
        auto version = *static_cast<DCCExCS::Version*>(parameter);
        lv_label_set_text_fmt(_csVersion, "Version: %s", version.version.c_str());
        lv_label_set_text_fmt(_csBoard, "Board: %s", version.board.c_str());
        lv_label_set_text_fmt(_csShield, "Shield: %s", version.shield.c_str());
        lv_label_set_text_fmt(_csBuild, "Build: %s", version.build.c_str());
        xSemaphoreGive(lvgl_mutex);
    }
  });

  _dccExCS.getCSVersion();
}

AboutUI::~AboutUI() {
  _dccExCS.removeEventListener(DCCExCS::Event::VERSION, _csVersionHandler);
  if (_container) lv_obj_del(_container);
}

void AboutUI::close_btn_event_cb(lv_event_t * e) {
  AboutUI* ui = (AboutUI*)lv_event_get_user_data(e);
  lv_obj_add_flag(ui->_container, LV_OBJ_FLAG_HIDDEN);
}
