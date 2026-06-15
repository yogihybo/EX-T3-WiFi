/* 
 * Based on XPT2046_Bitbang_Arduino_Library by ddxfish (https://github.com/ddxfish/XPT2046_Bitbang_Arduino_Library)
 * Modified to integrate with application Settings and remove SPIFFS dependency.
 */

#include "XPT2046_Bitbang.h"
#include <Settings.h>

#define CMD_READ_Y  0x90 // Command for XPT2046 to read Y position
#define CMD_READ_X  0xD0 // Command for XPT2046 to read X position

XPT2046_Bitbang::XPT2046_Bitbang(uint8_t mosiPin, uint8_t misoPin, uint8_t clkPin, uint8_t csPin, uint8_t irqPin) : 
    _mosiPin(mosiPin), _misoPin(misoPin), _clkPin(clkPin), _csPin(csPin), _irqPin(irqPin) {
}

void XPT2046_Bitbang::begin() {
    pinMode(_mosiPin, OUTPUT);
    pinMode(_misoPin, INPUT);
    pinMode(_clkPin, OUTPUT);
    pinMode(_csPin, OUTPUT);
    pinMode(_irqPin, INPUT); 
    digitalWrite(_csPin, HIGH);
    digitalWrite(_clkPin, LOW);
}

int XPT2046_Bitbang::readSPI(byte command) {
    int result = 0;

    for (int i = 7; i >= 0; i--) {
        digitalWrite(_mosiPin, command & (1 << i));
        digitalWrite(_clkPin, HIGH);
        delayMicroseconds(2);
        digitalWrite(_clkPin, LOW);
        delayMicroseconds(2);
    }

    // Dummy clock for BUSY bit
    digitalWrite(_clkPin, HIGH);
    delayMicroseconds(2);
    digitalWrite(_clkPin, LOW);
    delayMicroseconds(2);

    for (int i = 11; i >= 0; i--) {
        digitalWrite(_clkPin, HIGH);
        delayMicroseconds(2);
        result |= (digitalRead(_misoPin) << i);
        digitalWrite(_clkPin, LOW);
        delayMicroseconds(2);
    }

    return result;
}

bool XPT2046_Bitbang::isTouched() {
    // IRQ pin goes LOW when touched
    return digitalRead(_irqPin) == LOW;
}

Point XPT2046_Bitbang::getTouch() {
    digitalWrite(_csPin, LOW);
    int x = readSPI(CMD_READ_X);
    int y = readSPI(CMD_READ_Y);
    digitalWrite(_csPin, HIGH);
    
    return Point{x, y};
}
