#pragma once

#include <LVGL_CYD.h>
#include <DCCEXProtocol.h>
#include "LVGL_Layouts.h"

// CS info populated by AppDelegate on connect; read by AboutUI's update timer
struct CSInfo {
    String version;
    String board;
    String shield;
    String build;
    bool connected = false;
};
extern CSInfo csInfo;

class AboutUI : public UIView {
  private:
    lv_obj_t* _container;

    lv_obj_t* _csVersion;
    lv_obj_t* _csBoard;
    lv_obj_t* _csShield;
    lv_obj_t* _csBuild;
    lv_obj_t* _csConnected;

    lv_obj_t* _memLbl = nullptr;
    lv_obj_t* _wifiStat = nullptr;
    lv_obj_t* _sdStat = nullptr;
    lv_timer_t* _updateTimer = nullptr;

    static void update_timer_cb(lv_timer_t* timer);
    static void close_btn_event_cb(lv_event_t * e);

  public:
    AboutUI(DCCEXProtocol& dccex, lv_obj_t* parent);
    ~AboutUI() override;
};
