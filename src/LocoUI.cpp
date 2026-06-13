#include "LocoUI.h"
#include <SPIFFS.h>
#include <SD.h>
#include <StreamUtils.h>

LocoUI::LocoUI(DCCExCS& dccExCS, Locos& locos, lv_obj_t* parent) 
    : _dccExCS(dccExCS), _locos(locos), _loco((uint16_t)locos), _broadcastLocoHandler(0xFF), _fnPage(0) {
    
    _container = lv_obj_create(parent);
    lv_obj_set_size(_container, LV_PCT(100), LV_PCT(100));
    lv_obj_align(_container, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(_container, 0, 0);
    lv_obj_set_style_border_width(_container, 0, 0);
    lv_obj_clear_flag(_container, LV_OBJ_FLAG_SCROLLABLE);

    _keyboard = nullptr;
    _textarea = nullptr;

    if (_loco.address == 0) {
        buildControlScreen();
        lv_obj_clear_flag(_selectionMenu, LV_OBJ_FLAG_HIDDEN);
    } else {
        _broadcastLocoHandler = _dccExCS.addEventListener(DCCExCS::Event::BROADCAST_LOCO, [this](void* parameter) {
            if (xSemaphoreTake(lvgl_mutex, portMAX_DELAY) == pdTRUE) {
                broadcast(parameter);
                xSemaphoreGive(lvgl_mutex);
            }
        });
        buildControlScreen();
        _dccExCS.acquireLoco(_loco.address);
    }
}

LocoUI::~LocoUI() {
    if (_broadcastLocoHandler != 0xFF) {
        _dccExCS.removeEventListener(DCCExCS::Event::BROADCAST_LOCO, _broadcastLocoHandler);
    }
    if (_keyboard) lv_obj_del(_keyboard);
    if (_textarea) lv_obj_del(_textarea);
    if (_container) lv_obj_del(_container);
}

void LocoUI::broadcast(void* parameter) {
    auto broadcast = *static_cast<DCCExCS::Loco*>(parameter);
    if (_loco.address == broadcast.address) {
        if (_loco.speed != broadcast.speed) {
            lv_arc_set_value(_speedArc, broadcast.speed);
            lv_label_set_text_fmt(_speedLabel, "%d\nkm/h", broadcast.speed);
        }
        if (_loco.direction != broadcast.direction) {
            lv_label_set_text(_dirLabel, broadcast.direction ? "FWD" : "REV");
        }
        if (_loco.functions != broadcast.functions) {
            toggleFunctionButtons(_loco.functions ^ broadcast.functions);
        }
        _loco = broadcast;
    }
}

void LocoUI::buildSelectionMenu() {
    _selectionMenu = lv_obj_create(_container);
    lv_obj_set_size(_selectionMenu, LV_PCT(100), LV_PCT(100));
    lv_obj_align(_selectionMenu, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(_selectionMenu, 0, 0);
    lv_obj_set_style_border_width(_selectionMenu, 0, 0);
    lv_obj_set_style_bg_color(_selectionMenu, lv_color_make(30, 30, 30), 0);
    lv_obj_set_style_bg_opa(_selectionMenu, LV_OPA_COVER, 0);
    lv_obj_add_flag(_selectionMenu, LV_OBJ_FLAG_HIDDEN); // Hidden by default

    // Title Row
    lv_obj_t* title_row = lv_obj_create(_selectionMenu);
    lv_obj_set_width(title_row, LV_PCT(100));
    lv_obj_set_height(title_row, 40);
    lv_obj_set_style_pad_all(title_row, 0, 0);
    lv_obj_set_style_border_width(title_row, 0, 0);
    lv_obj_clear_flag(title_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(title_row);
    lv_label_set_text(title, "Select Locomotive");
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 10, 0);

    lv_obj_t* close_btn = lv_btn_create(title_row);
    lv_obj_align(close_btn, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_style_bg_color(close_btn, lv_color_make(200, 50, 50), 0);
    lv_obj_t* close_lbl = lv_label_create(close_btn);
    lv_label_set_text(close_lbl, "Back");
    lv_obj_center(close_lbl);
    lv_obj_add_event_cb(close_btn, close_selection_event_cb, LV_EVENT_CLICKED, this);

    lv_obj_t* btn_addr = lv_btn_create(_selectionMenu);
    lv_obj_set_size(btn_addr, 200, 50);
    lv_obj_align(btn_addr, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_t* lbl_addr = lv_label_create(btn_addr);
    lv_label_set_text(lbl_addr, "By Address");
    lv_obj_center(lbl_addr);
    lv_obj_add_event_cb(btn_addr, addr_btn_event_cb, LV_EVENT_CLICKED, this);

    lv_obj_t* btn_name = lv_btn_create(_selectionMenu);
    lv_obj_set_size(btn_name, 200, 50);
    lv_obj_align(btn_name, LV_ALIGN_TOP_MID, 0, 120);
    lv_obj_t* lbl_name = lv_label_create(btn_name);
    lv_label_set_text(lbl_name, "By Name");
    lv_obj_center(lbl_name);

    lv_obj_t* btn_group = lv_btn_create(_selectionMenu);
    lv_obj_set_size(btn_group, 200, 50);
    lv_obj_align(btn_group, LV_ALIGN_TOP_MID, 0, 180);
    lv_obj_t* lbl_group = lv_label_create(btn_group);
    lv_label_set_text(lbl_group, "By Group");
    lv_obj_center(lbl_group);
}

void LocoUI::buildControlScreen() {
    char path[32];
    sprintf(path, "/locos/%d.json", _loco.address);
    
    if (SPIFFS.exists(path)) {
        File locoFile = SPIFFS.open(path);
        ReadBufferingStream bufferedFile(locoFile, _locoDoc.capacity());
        deserializeJson(_locoDoc, bufferedFile);
        locoFile.close();
    } else if (SD.exists(path)) {
        File locoFile = SD.open(path);
        ReadBufferingStream bufferedFile(locoFile, _locoDoc.capacity());
        deserializeJson(_locoDoc, bufferedFile);
        locoFile.close();
    }

    String nameStr = "Unknown Loco";
    if (_locoDoc.containsKey("name")) nameStr = _locoDoc["name"].as<const char*>();

    // 1. Top Section (Name & Address Arrows)
    _nameLabel = lv_label_create(_container);
    lv_label_set_text(_nameLabel, nameStr.c_str());
    lv_obj_set_style_text_color(_nameLabel, lv_color_make(100, 100, 255), 0);
    lv_obj_align(_nameLabel, LV_ALIGN_TOP_MID, 0, 2);

    lv_obj_t* prev_btn = lv_btn_create(_container);
    lv_obj_set_size(prev_btn, 40, 30);
    lv_obj_align(prev_btn, LV_ALIGN_TOP_MID, -50, 20);
    lv_obj_t* pl = lv_label_create(prev_btn);
    lv_label_set_text(pl, "<");
    lv_obj_center(pl);
    lv_obj_add_event_cb(prev_btn, nav_btn_event_cb, LV_EVENT_CLICKED, this);
    lv_obj_set_user_data(prev_btn, (void*)0);

    _addressLabel = lv_label_create(_container);
    lv_label_set_text_fmt(_addressLabel, "%d", _loco.address);
    lv_obj_set_style_text_font(_addressLabel, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(_addressLabel, lv_color_make(100, 100, 255), 0);
    lv_obj_align(_addressLabel, LV_ALIGN_TOP_MID, 0, 24);

    lv_obj_t* next_btn = lv_btn_create(_container);
    lv_obj_set_size(next_btn, 40, 30);
    lv_obj_align(next_btn, LV_ALIGN_TOP_MID, 50, 20);
    lv_obj_t* nl = lv_label_create(next_btn);
    lv_label_set_text(nl, ">");
    lv_obj_center(nl);
    lv_obj_add_event_cb(next_btn, nav_btn_event_cb, LV_EVENT_CLICKED, this);
    lv_obj_set_user_data(next_btn, (void*)1);

    // 2. Central Speedometer (lv_arc)
    _speedArc = lv_arc_create(_container);
    lv_obj_set_size(_speedArc, 130, 130);
    lv_arc_set_rotation(_speedArc, 135);
    lv_arc_set_bg_angles(_speedArc, 0, 270);
    lv_arc_set_range(_speedArc, 0, 126);
    lv_arc_set_value(_speedArc, _loco.speed);
    lv_obj_align(_speedArc, LV_ALIGN_CENTER, 0, -5);
    lv_obj_add_event_cb(_speedArc, speed_arc_event_cb, LV_EVENT_VALUE_CHANGED, this);

    _speedLabel = lv_label_create(_speedArc);
    lv_label_set_text_fmt(_speedLabel, "%d\nkm/h", _loco.speed);
    lv_obj_set_style_text_align(_speedLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(_speedLabel, &lv_font_montserrat_16, 0);
    lv_obj_center(_speedLabel);

    // 3. Bottom Tool Buttons (E-Stop, FWD/REV, F>>)
    lv_obj_t* estop_btn = lv_btn_create(_container);
    lv_obj_set_size(estop_btn, 50, 35);
    lv_obj_align(estop_btn, LV_ALIGN_BOTTOM_LEFT, 2, -2);
    lv_obj_set_style_bg_color(estop_btn, lv_color_make(255, 50, 50), 0);
    lv_obj_t* el = lv_label_create(estop_btn);
    lv_label_set_text(el, "STOP");
    lv_obj_center(el);
    lv_obj_add_event_cb(estop_btn, estop_btn_event_cb, LV_EVENT_CLICKED, this);

    _dirBtn = lv_btn_create(_container);
    lv_obj_set_size(_dirBtn, 60, 35);
    lv_obj_align(_dirBtn, LV_ALIGN_BOTTOM_MID, 0, -2);
    _dirLabel = lv_label_create(_dirBtn);
    lv_label_set_text(_dirLabel, _loco.direction ? "FWD" : "REV");
    lv_obj_center(_dirLabel);
    lv_obj_add_event_cb(_dirBtn, dir_btn_event_cb, LV_EVENT_CLICKED, this);

    lv_obj_t* page_btn = lv_btn_create(_container);
    lv_obj_set_size(page_btn, 50, 35);
    lv_obj_align(page_btn, LV_ALIGN_BOTTOM_RIGHT, -2, -2);
    lv_obj_set_style_bg_color(page_btn, lv_color_make(50, 50, 50), 0);
    lv_obj_t* pl2 = lv_label_create(page_btn);
    lv_label_set_text(pl2, "F>>");
    lv_obj_center(pl2);
    lv_obj_add_event_cb(page_btn, page_btn_event_cb, LV_EVENT_CLICKED, this);

    // 4. Build Function Buttons
    buildFunctionButtons();
    renderFunctionPage();
}

void LocoUI::buildFunctionButtons() {
    if (_locoDoc["functions"].is<JsonArray>()) {
        _locoFunctions = _locoDoc["functions"].as<JsonArray>();
    } else {
        File functionFile;
        if (_locoDoc.containsKey("functions")) {
            const char* fnPath = _locoDoc["functions"].as<const char*>();
            if (SPIFFS.exists(fnPath)) functionFile = SPIFFS.open(fnPath);
            else if (SD.exists(fnPath)) functionFile = SD.open(fnPath);
        }
        if (!functionFile) functionFile = SPIFFS.open("/default.json");

        StaticJsonDocument<16> filter;
        filter["functions"] = true;
        ReadBufferingStream bufferedFile(functionFile, _locoDoc.capacity());
        deserializeJson(_locoDoc, functionFile, DeserializationOption::Filter(filter));
        _locoFunctions = _locoDoc["functions"].as<JsonArray>();
        if(functionFile) functionFile.close();
    }

    for (JsonArrayConst const& row : _locoFunctions) {
        for (JsonObjectConst const& fn : row) {
            uint8_t func = fn["fn"];
            bool latching = fn["latching"] | true;
            const char* label = fn["btn"]["idle"]["label"].as<const char*>();
            
            lv_obj_t* btn = lv_btn_create(_container);
            lv_obj_set_size(btn, 42, 30);
            
            if (latching) lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
            if (_loco.functions.test(func)) lv_obj_add_state(btn, LV_STATE_CHECKED);

            lv_obj_t* lbl = lv_label_create(btn);
            lv_label_set_text_fmt(lbl, "F%d", func); // Fallback to F%d
            if (label) {
                // If it's a known symbol or short text, we can use it
                lv_label_set_text(lbl, label); 
            }
            lv_obj_center(lbl);

            lv_obj_set_user_data(btn, (void*)(uintptr_t)func);
            lv_obj_add_event_cb(btn, fn_btn_event_cb, LV_EVENT_VALUE_CHANGED, this);
            lv_obj_add_event_cb(btn, fn_btn_event_cb, LV_EVENT_CLICKED, this);
            
            lv_obj_add_flag(btn, LV_OBJ_FLAG_HIDDEN); // Hidden initially
            _fnButtons.push_back(btn);
        }
    }
}

void LocoUI::renderFunctionPage() {
    for (auto btn : _fnButtons) {
        lv_obj_add_flag(btn, LV_OBJ_FLAG_HIDDEN);
    }

    int start_idx = _fnPage * 10;
    for (int i = 0; i < 10; i++) {
        int idx = start_idx + i;
        if (idx >= _fnButtons.size()) break;

        lv_obj_t* btn = _fnButtons[idx];
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_HIDDEN);

        // Calculate position
        // Left column (i < 5), Right column (i >= 5)
        int y_pos = 50 + (i % 5) * 32;
        if (i < 5) {
            lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 2, y_pos);
        } else {
            lv_obj_align(btn, LV_ALIGN_TOP_RIGHT, -2, y_pos);
        }
    }
}

void LocoUI::toggleFunctionButtons(std::bitset<32> toggle) {
    for (auto child : _fnButtons) {
        uint8_t func = (uintptr_t)lv_obj_get_user_data(child);
        if (toggle.test(func)) {
            if (_loco.functions.test(func)) {
                lv_obj_add_state(child, LV_STATE_CHECKED);
            } else {
                lv_obj_clear_state(child, LV_STATE_CHECKED);
            }
        }
    }
}

void LocoUI::showKeypad() {
  if (!_keyboard) {
    _textarea = lv_textarea_create(_container);
    lv_obj_set_width(_textarea, LV_PCT(100));
    lv_obj_align(_textarea, LV_ALIGN_TOP_MID, 0, 10);
    lv_textarea_set_one_line(_textarea, true);
    lv_textarea_set_placeholder_text(_textarea, "Loco Address");

    _keyboard = lv_keyboard_create(_container);
    lv_keyboard_set_mode(_keyboard, LV_KEYBOARD_MODE_NUMBER);
    lv_keyboard_set_textarea(_keyboard, _textarea);
    lv_obj_add_event_cb(_keyboard, kb_event_cb, LV_EVENT_READY, this);
    lv_obj_add_event_cb(_keyboard, kb_event_cb, LV_EVENT_CANCEL, this);
  }
}

void LocoUI::hideKeypad() {
  if (_keyboard) { lv_obj_delete_async(_keyboard); _keyboard = nullptr; }
  if (_textarea) { lv_obj_delete_async(_textarea); _textarea = nullptr; }
}

void LocoUI::refresh() {
    lv_obj_clean(_container);
    _fnButtons.clear();
    _loco.address = (uint16_t)_locos;
    
    if (_loco.address == 0) {
        buildControlScreen();
        lv_obj_clear_flag(_selectionMenu, LV_OBJ_FLAG_HIDDEN);
    } else {
        if (_broadcastLocoHandler == 0xFF) {
            _broadcastLocoHandler = _dccExCS.addEventListener(DCCExCS::Event::BROADCAST_LOCO, [this](void* parameter) {
                if (xSemaphoreTake(lvgl_mutex, portMAX_DELAY) == pdTRUE) {
                    broadcast(parameter);
                    xSemaphoreGive(lvgl_mutex);
                }
            });
        }
        buildControlScreen();
        _dccExCS.acquireLoco(_loco.address);
    }
}

void LocoUI::open_selection_event_cb(lv_event_t * e) {
    LocoUI* ui = (LocoUI*)lv_event_get_user_data(e);
    if (ui->_selectionMenu) {
        lv_obj_clear_flag(ui->_selectionMenu, LV_OBJ_FLAG_HIDDEN);
    }
}

void LocoUI::close_selection_event_cb(lv_event_t * e) {
    LocoUI* ui = (LocoUI*)lv_event_get_user_data(e);
    if (ui->_selectionMenu) {
        lv_obj_add_flag(ui->_selectionMenu, LV_OBJ_FLAG_HIDDEN);
    }
}

void LocoUI::addr_btn_event_cb(lv_event_t * e) {
    LocoUI* ui = (LocoUI*)lv_event_get_user_data(e);
    if (ui->_selectionMenu) lv_obj_add_flag(ui->_selectionMenu, LV_OBJ_FLAG_HIDDEN);
    ui->showKeypad();
}

void LocoUI::kb_event_cb(lv_event_t * e) {
    LocoUI* ui = (LocoUI*)lv_event_get_user_data(e);
    if (lv_event_get_code(e) == LV_EVENT_READY) {
        const char* txt = lv_textarea_get_text(ui->_textarea);
        if (txt && strlen(txt) > 0) {
            uint16_t addr = atoi(txt);
            if (addr > 0 && addr <= 9999) {
                ui->_locos.add(addr);
                lv_async_call([](void* user_data) {
                    ((LocoUI*)user_data)->refresh();
                }, ui);
                return;
            }
        }
    }
    ui->hideKeypad();
}

void LocoUI::nav_btn_event_cb(lv_event_t * e) {
    LocoUI* ui = (LocoUI*)lv_event_get_user_data(e);
    intptr_t action = (intptr_t)lv_obj_get_user_data((lv_obj_t*)lv_event_get_target(e));
    if (action == 0) ui->_locos.prev();
    else ui->_locos.next();
    ui->refresh();
}

void LocoUI::dir_btn_event_cb(lv_event_t * e) {
    LocoUI* ui = (LocoUI*)lv_event_get_user_data(e);
    ui->_dccExCS.setLocoThrottle(ui->_loco.address, ui->_loco.speed, !ui->_loco.direction);
}

void LocoUI::speed_arc_event_cb(lv_event_t * e) {
    LocoUI* ui = (LocoUI*)lv_event_get_user_data(e);
    lv_obj_t* arc = (lv_obj_t*)lv_event_get_target(e);
    int speed = lv_arc_get_value(arc);
    ui->_dccExCS.setLocoThrottle(ui->_loco.address, speed, ui->_loco.direction);
}

void LocoUI::fn_btn_event_cb(lv_event_t * e) {
    LocoUI* ui = (LocoUI*)lv_event_get_user_data(e);
    lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
    uint8_t func = (uintptr_t)lv_obj_get_user_data(btn);
    bool is_checked = lv_obj_has_state(btn, LV_STATE_CHECKED);
    ui->_dccExCS.setLocoFn(ui->_loco.address, func, is_checked);
}

void LocoUI::page_btn_event_cb(lv_event_t * e) {
    LocoUI* ui = (LocoUI*)lv_event_get_user_data(e);
    ui->_fnPage++;
    if (ui->_fnPage * 10 >= ui->_fnButtons.size()) ui->_fnPage = 0;
    ui->renderFunctionPage();
}

void LocoUI::estop_btn_event_cb(lv_event_t * e) {
    LocoUI* ui = (LocoUI*)lv_event_get_user_data(e);
    ui->_dccExCS.setLocoThrottle(ui->_loco.address, 0, ui->_loco.direction);
}
