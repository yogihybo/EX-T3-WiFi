#include "PowerUI.h"

PowerUI::PowerUI(DCCEXProtocol& dccex, lv_obj_t* parent)
    : _dccex(dccex) {

  _container = lv_obj_create(parent);
  lv_obj_set_size(_container, LV_PCT(100), LV_PCT(100));
  lv_obj_align(_container, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_pad_all(_container, 0, 0);
  lv_obj_set_style_border_width(_container, 0, 0);

  lv_obj_t* title = lv_label_create(_container);
  lv_label_set_text(title, "Track Power Control");
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

  auto create_btn = [this](int x, int y, int w, int h, const char* label, int action_id) {
    lv_obj_t* btn = lv_btn_create(_container);
    lv_obj_set_size(btn, w, h);
    lv_obj_align(btn, LV_ALIGN_TOP_MID, x, y);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_t* lbl = lv_label_create(btn);
    lv_label_set_text(lbl, label);
    lv_obj_center(lbl);
    lv_obj_set_user_data(btn, (void*)(uintptr_t)action_id);
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_VALUE_CHANGED, this);
    return btn;
  };

  _powerMain = create_btn(-55, 40, 100, 45, "Main", 1);
  _powerProg = create_btn(55, 40, 100, 45, "Prog", 2);
  _powerAll  = create_btn(0, 95, 210, 45, "All Tracks", 3);
  _powerJoin = create_btn(0, 150, 210, 45, "Join Tracks", 4);
}

PowerUI::~PowerUI() {
  if (_container) lv_obj_del(_container);
}

void PowerUI::onPowerUpdate(bool main, bool prog, bool join) {
  _updatingFromBroadcast = true;

  Serial.printf("[DCC] Track power – Main: %s  Prog: %s  Join: %s\n",
      main ? "ON" : "OFF", prog ? "ON" : "OFF", join ? "ON" : "OFF");

  if (_powerMain) {
    if (main) lv_obj_add_state(_powerMain, LV_STATE_CHECKED);
    else      lv_obj_clear_state(_powerMain, LV_STATE_CHECKED);
  }
  if (_powerProg) {
    if (prog) lv_obj_add_state(_powerProg, LV_STATE_CHECKED);
    else      lv_obj_clear_state(_powerProg, LV_STATE_CHECKED);
  }
  if (_powerAll) {
    if (main && prog) lv_obj_add_state(_powerAll, LV_STATE_CHECKED);
    else              lv_obj_clear_state(_powerAll, LV_STATE_CHECKED);
  }
  if (_powerJoin) {
    if (join) lv_obj_add_state(_powerJoin, LV_STATE_CHECKED);
    else      lv_obj_clear_state(_powerJoin, LV_STATE_CHECKED);
  }

  _updatingFromBroadcast = false;
}

void PowerUI::onIndividualPowerUpdate(TrackPower state, int track) {
  bool on = (state == TrackPower::PowerOn);
  _updatingFromBroadcast = true;

  // track: 2698315=MAIN, 2788330=PROG, 65..72='A'..'H' (see DCCEXProtocol.h comment)
  if (track == 2698315 && _powerMain) {
    if (on) lv_obj_add_state(_powerMain, LV_STATE_CHECKED);
    else    lv_obj_clear_state(_powerMain, LV_STATE_CHECKED);
  } else if (track == 2788330 && _powerProg) {
    if (on) lv_obj_add_state(_powerProg, LV_STATE_CHECKED);
    else    lv_obj_clear_state(_powerProg, LV_STATE_CHECKED);
  }

  _updatingFromBroadcast = false;
}

void PowerUI::btn_event_cb(lv_event_t * e) {
  PowerUI* ui = (PowerUI*)lv_event_get_user_data(e);
  if (ui->_updatingFromBroadcast) return;

  lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
  int action = (int)(uintptr_t)lv_obj_get_user_data(btn);
  bool state = lv_obj_has_state(btn, LV_STATE_CHECKED);

  if      (action == 1) { if (state) ui->_dccex.powerMainOn();  else ui->_dccex.powerMainOff(); }
  else if (action == 2) { if (state) ui->_dccex.powerProgOn();  else ui->_dccex.powerProgOff(); }
  else if (action == 3) { if (state) ui->_dccex.powerOn();    else ui->_dccex.powerOff(); }
  else if (action == 4) { if (state) ui->_dccex.joinProg();   else ui->_dccex.powerOff(); }
}
