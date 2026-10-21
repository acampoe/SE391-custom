#pragma once

#include <Arduino.h>

class LC78212
{
public:
    enum class Switch : uint8_t
    {
        None = 0,
        Phono = 1,
        Cd = 2,
        Tuner = 3,
        Video = 4,
        TvLd = 5,
        MdTape = 6
    };

    void begin();
    void select(Switch selected);
    void setMask(uint8_t mask);
    void allOff();

private:
    // Deliberately slow for the hand-wired, 10k pull-up transistor interface.
    // This remains far inside the device's static serial-interface capability.
    static constexpr uint16_t HALF_CLOCK_US = 100;

    static void writePhysical(uint8_t gpio, bool high);
    static void pulseClock();
    static void sendBit(bool bit);
    void writeFrame(uint8_t mask, bool addressA0);
    void writeMask(uint8_t mask);
};
