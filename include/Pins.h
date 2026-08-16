#pragma once

#include <Arduino.h>

namespace Pins
{
    constexpr uint8_t BUTTON_POWER = 4;
    constexpr uint8_t BUTTON_DISPLAY = 25;
    constexpr uint8_t LED_FIVE_ONE_CH = 21;
    constexpr uint8_t LED_BASS_BOOST = 22;
    constexpr uint8_t LED_MUTING = 13;

    // LC78212 control signals pass through inverting NPN open-collector
    // level shifters. These constants name the ESP32 side of the stages.
    constexpr uint8_t INPUT_SWITCH_CE = 18;
    constexpr uint8_t INPUT_SWITCH_DI = 19;
    constexpr uint8_t INPUT_SWITCH_CL = 23;

    // PT6312B pins will be assigned after its bench test. GPIO18 and GPIO23
    // are now occupied by the input selector and must not be reused.

    // GPIO26 is used by the keypad ADC and cannot also drive the main relay.
    // Assign RELAY_MAIN after the relay wiring is moved to a free output pin.
}
