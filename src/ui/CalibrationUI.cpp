#include "CalibrationUI.h"
#include <Settings.h>

CalibrationUI::CalibrationUI() {
    _container = nullptr;
    _instructions = nullptr;
    _target = nullptr;
    _pollTimer = nullptr;
    _state = 0;
}

CalibrationUI::~CalibrationUI() {
    hide();
}

void CalibrationUI::show() {
    if (_container) return;

    _state = 0;

    // Create full screen overlay
    _container = lv_obj_create(lv_layer_top());
    lv_obj_set_size(_container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(_container, lv_color_make(0, 0, 0), 0);
    lv_obj_set_style_bg_opa(_container, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(_container, 0, 0);
    lv_obj_set_style_radius(_container, 0, 0);

    _instructions = lv_label_create(_container);
    lv_obj_set_style_text_color(_instructions, lv_color_make(255, 255, 255), 0);
    lv_label_set_text(_instructions, "Touch Calibration\n\nTap the target in the Top Left.");
    lv_obj_center(_instructions);

    _target = lv_obj_create(_container);
    lv_obj_set_size(_target, 20, 20);
    lv_obj_set_style_bg_color(_target, lv_color_make(255, 0, 0), 0);
    lv_obj_set_style_radius(_target, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(_target, 2, 0);
    lv_obj_set_style_border_color(_target, lv_color_make(255, 255, 255), 0);

    drawTarget(10, 10);

    // Create high frequency timer to poll the raw touch driver directly
    _pollTimer = lv_timer_create(poll_timer_cb, 50, this);
}

void CalibrationUI::hide() {
    if (_pollTimer) {
        lv_timer_del(_pollTimer);
        _pollTimer = nullptr;
    }
    if (_container) {
        lv_obj_del(_container);
        _container = nullptr;
    }
}

void CalibrationUI::drawTarget(int x, int y) {
    lv_obj_set_pos(_target, x - 10, y - 10);
}

void CalibrationUI::poll_timer_cb(lv_timer_t* timer) {
    CalibrationUI* ui = (CalibrationUI*)lv_timer_get_user_data(timer);
    if (ui) ui->processTouch();
}

void CalibrationUI::processTouch() {
    bool isTouched = touchscreen.isTouched();

    if (_state == 0) { // Wait for Top Left Press
        if (isTouched) {
            Point p = touchscreen.getTouch();
            _rx1 = p.x;
            _ry1 = p.y;
            _state = 1;
            lv_label_set_text(_instructions, "Release...");
            lv_obj_set_style_bg_color(_target, lv_color_make(0, 255, 0), 0);
        }
    } else if (_state == 1) { // Wait for Release
        if (!isTouched) {
            _state = 2;
            lv_label_set_text(_instructions, "Touch Calibration\n\nTap the target in the Bottom Right.");
            lv_obj_set_style_bg_color(_target, lv_color_make(255, 0, 0), 0);
            drawTarget(230, 310);
        }
    } else if (_state == 2) { // Wait for Bottom Right Press
        if (isTouched) {
            Point p = touchscreen.getTouch();
            _rx2 = p.x;
            _ry2 = p.y;
            _state = 3;
            lv_label_set_text(_instructions, "Release...");
            lv_obj_set_style_bg_color(_target, lv_color_make(0, 255, 0), 0);
        }
    } else if (_state == 3) { // Wait for Final Release
        if (!isTouched) {
            // Both points recorded! Calculate bounds.
            // Target 1 was at cx=10, cy=10
            // Target 2 was at cx=230, cy=310
            // dx = 220, dy = 300

            // p.x mapped to 239..0
            // p.y mapped to 0..319

            // Calculate xMin (maps to 239) and xMax (maps to 0)
            int dx_raw = _rx2 - _rx1;
            int xMax = _rx1 - 10 * dx_raw / 220;
            int xMin = _rx2 + 9 * dx_raw / 220;

            // Calculate yMin (maps to 0) and yMax (maps to 319)
            int dy_raw = _ry2 - _ry1;
            int yMin = _ry1 - 10 * dy_raw / 300;
            int yMax = _ry2 + 9 * dy_raw / 300;

            // Debug Output
            Serial.println("--- Touch Calibration Complete ---");
            Serial.printf("Point 1 (Top Left): Raw X=%d, Y=%d\n", _rx1, _ry1);
            Serial.printf("Point 2 (Bottom Right): Raw X=%d, Y=%d\n", _rx2, _ry2);
            
            Serial.printf("Old Bounds - xMin: %d, xMax: %d, yMin: %d, yMax: %d\n", 
                Settings.TouchCal.xMin, Settings.TouchCal.xMax, Settings.TouchCal.yMin, Settings.TouchCal.yMax);
            
            Serial.printf("New Bounds - xMin: %d, xMax: %d, yMin: %d, yMax: %d\n", 
                xMin, xMax, yMin, yMax);
                
            Serial.printf("Adjustments - dxMin: %d, dxMax: %d, dyMin: %d, dyMax: %d\n", 
                xMin - Settings.TouchCal.xMin, 
                xMax - Settings.TouchCal.xMax, 
                yMin - Settings.TouchCal.yMin, 
                yMax - Settings.TouchCal.yMax);
            Serial.println("----------------------------------");

            Settings.TouchCal.xMin = xMin;
            Settings.TouchCal.xMax = xMax;
            Settings.TouchCal.yMin = yMin;
            Settings.TouchCal.yMax = yMax;
            Settings.save();

            hide();
        }
    }
}
