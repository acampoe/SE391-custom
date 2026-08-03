#pragma once

#include <Arduino.h>

namespace Config
{
    constexpr auto FW_NAME = "Sony STR-SE391 Custom Receiver";
    constexpr auto FW_VERSION = "0.1.0";

    constexpr uint32_t SERIAL_BAUD = 9600;

    constexpr bool POWER_BUTTON_ACTIVE_LOW = true;
    constexpr uint16_t POWER_BUTTON_DEBOUNCE_MS = 35;
}
