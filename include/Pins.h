#pragma once

#include <Arduino.h>

namespace Pins
{
    constexpr uint8_t BUTTON_POWER = 4;
    constexpr uint8_t BUTTON_DISPLAY = 25;
    constexpr uint8_t LED_FIVE_ONE_CH = 21;

    constexpr uint8_t VFD_DATA  = 23;
    constexpr uint8_t VFD_CLOCK = 18;
    constexpr uint8_t VFD_STB   = 5;

    // GPIO26 is used by the keypad ADC and cannot also drive the main relay.
    // Assign RELAY_MAIN after the relay wiring is moved to a free output pin.
}
