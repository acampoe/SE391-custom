#include "LC78212.h"

#include "Pins.h"

void LC78212::writePhysical(uint8_t gpio, bool high)
{
    // The NPN stage inverts: ESP32 LOW produces a 5 V logic HIGH at IC401.
    digitalWrite(gpio, high ? LOW : HIGH);
}

void LC78212::begin()
{
    // Establish inactive physical levels before enabling the GPIO drivers.
    // CE falling latches data, so it remains low while idle.
    digitalWrite(Pins::INPUT_SWITCH_CE, HIGH);
    digitalWrite(Pins::INPUT_SWITCH_DI, HIGH);
    digitalWrite(Pins::INPUT_SWITCH_CL, HIGH);
    pinMode(Pins::INPUT_SWITCH_CE, OUTPUT);
    pinMode(Pins::INPUT_SWITCH_DI, OUTPUT);
    pinMode(Pins::INPUT_SWITCH_CL, OUTPUT);

    delayMicroseconds(HALF_CLOCK_US);
    allOff();
}

void LC78212::pulseClock()
{
    writePhysical(Pins::INPUT_SWITCH_CL, false);
    delayMicroseconds(HALF_CLOCK_US);
    writePhysical(Pins::INPUT_SWITCH_CL, true);
    delayMicroseconds(HALF_CLOCK_US);
    writePhysical(Pins::INPUT_SWITCH_CL, false);
    delayMicroseconds(HALF_CLOCK_US);
}

void LC78212::sendBit(bool bit)
{
    writePhysical(Pins::INPUT_SWITCH_DI, bit);
    delayMicroseconds(HALF_CLOCK_US);
    pulseClock();
}

void LC78212::writeMask(uint8_t mask)
{
    writePhysical(Pins::INPUT_SWITCH_CL, false);
    writePhysical(Pins::INPUT_SWITCH_CE, true);
    delayMicroseconds(HALF_CLOCK_US);

    // The device receives A0..A3 first, followed by D1..D8. Data is sampled
    // on CL rising edges and transferred to the switches on CE's falling edge.
    sendBit(S_PIN_HIGH); // A0
    sendBit(false);      // A1
    sendBit(true);       // A2
    sendBit(true);       // A3

    for (uint8_t bit = 0; bit < 8; ++bit)
        sendBit(mask & (1U << bit));

    writePhysical(Pins::INPUT_SWITCH_DI, false);
    delayMicroseconds(HALF_CLOCK_US);
    writePhysical(Pins::INPUT_SWITCH_CE, false);
    delayMicroseconds(HALF_CLOCK_US);
}

void LC78212::select(Switch selected)
{
    const uint8_t switchNumber = static_cast<uint8_t>(selected);
    writeMask(switchNumber == 0 ? 0 : (1U << (switchNumber - 1)));
}

void LC78212::allOff()
{
    writeMask(0);
}
