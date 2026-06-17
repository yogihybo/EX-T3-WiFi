#pragma once

#include <LVGL_CYD.h>
#include <DCCEXProtocol.h>
#include "LVGL_Layouts.h"

class PowerUI : public UIView {
  private:
    DCCEXProtocol& _dccex;
    bool _updatingFromBroadcast = false;
    bool _mainOn = false;
    bool _progOn = false;

    lv_obj_t* _container;
    lv_obj_t* _powerAll;
    lv_obj_t* _powerMain;
    lv_obj_t* _powerProg;
    lv_obj_t* _powerJoin;

    static void btn_event_cb(lv_event_t * e);

  public:
    PowerUI(DCCEXProtocol& dccex, lv_obj_t* parent);
    ~PowerUI() override;

    void onPowerUpdate(bool main, bool prog, bool join);
    void onIndividualPowerUpdate(TrackPower state, int track);
};
