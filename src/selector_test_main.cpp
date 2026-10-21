#include <Arduino.h>

#include "LC78212.h"
#include "Pins.h"

namespace {
LC78212 selector;

void setPhysical(uint8_t gpio, bool high)
{
    // NPN level shifter inversion: ESP32 LOW gives HIGH at LC78212.
    digitalWrite(gpio, high ? LOW : HIGH);
}

void holdLine(const char* name, uint8_t gpio, bool high)
{
    Serial.printf("[MEASURE] %s should be %s for 5 seconds\n",
                  name, high ? "HIGH (~5 V)" : "LOW (~0 V)");
    setPhysical(gpio, high);
    delay(5000);
}

void runLineTest()
{
    setPhysical(Pins::INPUT_SWITCH_CE, false);
    setPhysical(Pins::INPUT_SWITCH_DI, false);
    setPhysical(Pins::INPUT_SWITCH_CL, false);

    holdLine("CE pin 13", Pins::INPUT_SWITCH_CE, true);
    holdLine("CE pin 13", Pins::INPUT_SWITCH_CE, false);
    holdLine("DI pin 14", Pins::INPUT_SWITCH_DI, true);
    holdLine("DI pin 14", Pins::INPUT_SWITCH_DI, false);
    holdLine("CL pin 15", Pins::INPUT_SWITCH_CL, true);
    holdLine("CL pin 15", Pins::INPUT_SWITCH_CL, false);
}

void sendSlowBit(const char* name, bool value)
{
    Serial.printf("[TRACE] %s: DI=%u, CL=LOW\n", name, value ? 1 : 0);
    setPhysical(Pins::INPUT_SWITCH_DI, value);
    setPhysical(Pins::INPUT_SWITCH_CL, false);
    delay(1000);

    Serial.printf("[TRACE] %s: CL=HIGH (sample edge)\n", name);
    setPhysical(Pins::INPUT_SWITCH_CL, true);
    delay(1000);

    Serial.printf("[TRACE] %s: CL=LOW\n", name);
    setPhysical(Pins::INPUT_SWITCH_CL, false);
    delay(1000);
}

void runSlowAllOnTrace()
{
    Serial.println("[TRACE] Starting slow LC78212 address 0x0C / all-on frame");
    Serial.println("[TRACE] CE=LOW for address A0..A3");
    setPhysical(Pins::INPUT_SWITCH_CE, false);
    setPhysical(Pins::INPUT_SWITCH_CL, false);
    delay(2000);

    sendSlowBit("A0", false);
    sendSlowBit("A1", false);
    sendSlowBit("A2", true);
    sendSlowBit("A3", true);

    Serial.println("[TRACE] CE=HIGH for data D1..D8");
    setPhysical(Pins::INPUT_SWITCH_CE, true);
    delay(2000);

    sendSlowBit("D1", true);
    sendSlowBit("D2", true);
    sendSlowBit("D3", true);
    sendSlowBit("D4", true);
    sendSlowBit("D5", true);
    sendSlowBit("D6", true);
    sendSlowBit("D7", true);
    sendSlowBit("D8", true);

    Serial.println("[TRACE] CE=LOW now: all eight switches should latch ON");
    setPhysical(Pins::INPUT_SWITCH_CE, false);
    setPhysical(Pins::INPUT_SWITCH_DI, false);
    setPhysical(Pins::INPUT_SWITCH_CL, false);
    Serial.println("[MEASURE] Pin 2 and pin 4 now; holding for 10 seconds");
    delay(10000);
}
}

void setup()
{
    Serial.begin(9600);
    delay(1500);

    // Start every transistor conducting so all IC control inputs are low.
    digitalWrite(Pins::INPUT_SWITCH_CE, HIGH);
    digitalWrite(Pins::INPUT_SWITCH_DI, HIGH);
    digitalWrite(Pins::INPUT_SWITCH_CL, HIGH);
    pinMode(Pins::INPUT_SWITCH_CE, OUTPUT);
    pinMode(Pins::INPUT_SWITCH_DI, OUTPUT);
    pinMode(Pins::INPUT_SWITCH_CL, OUTPUT);

    Serial.println("\n========================================");
    Serial.println(" LC78212 SELECTOR DIAGNOSTIC");
    Serial.println("========================================");
    runLineTest();
    runSlowAllOnTrace();

    selector.begin();
    Serial.println("[INFO] Control-line test complete.");
    Serial.println("[INFO] Cycling switches 1 through 8 every 5 seconds.");
}

void loop()
{
    Serial.println("[SELECT] ALL eight switches ON for 10 seconds");
    selector.setMask(0xFF);
    delay(10000);

    Serial.println("[SELECT] All switches OFF for 5 seconds");
    selector.allOff();
    delay(5000);

    for (uint8_t number = 1; number <= 8; ++number) {
        Serial.printf("[SELECT] Switch %u ON\n", number);
        selector.select(static_cast<LC78212::Switch>(number));
        delay(5000);
    }

    Serial.println("[SELECT] All switches OFF");
    selector.allOff();
    delay(5000);
}
