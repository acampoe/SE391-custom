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
    void allOff();

private:
    // Sony connects LC78212 pin 17 (S) low, selecting address A0..A3=0011.
    static constexpr bool S_PIN_HIGH = false;
    static constexpr uint16_t HALF_CLOCK_US = 5;

    static void writePhysical(uint8_t gpio, bool high);
    static void pulseClock();
    static void sendBit(bool bit);
    void writeMask(uint8_t mask);
};
