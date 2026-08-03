#include <Arduino.h>
#include "Logger.h"
#include "Config.h"

void Logger::begin()
{
    Serial.begin(Config::SERIAL_BAUD);

    // Give the serial port a moment to initialize
    delay(100);

    Serial.println();
    Serial.println("================================");
    Serial.println(Config::FW_NAME);
    Serial.print("Firmware ");
    Serial.println(Config::FW_VERSION);
    Serial.println("================================");
}

void Logger::info(const char* msg)
{
    Serial.print("[INFO] ");
    Serial.println(msg);
}

void Logger::warning(const char* msg)
{
    Serial.print("[WARN] ");
    Serial.println(msg);
}

void Logger::error(const char* msg)
{
    Serial.print("[ERROR] ");
    Serial.println(msg);
}