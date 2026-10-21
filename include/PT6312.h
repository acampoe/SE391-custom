#pragma once

#include <Arduino.h>

class PT6312
{
public:
    PT6312(uint8_t dinPin, uint8_t clockPin, uint8_t strobePin);

    // Initializes 8-grid/14-segment mode, clears RAM, and leaves the
    // high-voltage display outputs disabled for safe bench testing.
    void begin();
    void clear();
    void setDisplayEnabled(bool enabled, uint8_t brightness = 0);
    void showFirstIntersection();
    void showIntersection(uint8_t grid, uint8_t segment);
    void showChatGpt();
    void showWelcome();
    void showCd();
    void showTextWindow(const char *text, int16_t offset);

private:
    void showGridPatterns(const uint16_t patterns[8]);
    void sendCommand(uint8_t command);
    void writeByteLsbFirst(uint8_t value);

    uint8_t dinPin_;
    uint8_t clockPin_;
    uint8_t strobePin_;
};
