#pragma once

#include <Arduino.h>

namespace Pins
{
    constexpr uint8_t BUTTON_POWER = 27;
    constexpr uint8_t RELAY_MAIN = 17;

    // PT6312 three-wire interface. These are logic inputs on the PT6312.
    constexpr uint8_t PT6312_DIN = 18;
    constexpr uint8_t PT6312_CLK = 19;
    constexpr uint8_t PT6312_STB = 23;

    // LC78212 control lines use NPN inverters with 10k pull-ups to +5 V.
    constexpr uint8_t INPUT_SWITCH_CE = 21;
    constexpr uint8_t INPUT_SWITCH_DI = 22;
    constexpr uint8_t INPUT_SWITCH_CL = 25;
}
