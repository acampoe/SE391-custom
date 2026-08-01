#include <Arduino.h>
#include "Receiver.h"

void Receiver::begin()
{
    Serial.begin(115200);

    Serial.println();
    Serial.println("===============================");
    Serial.println(" Sony STR-SE391 Custom Receiver");
    Serial.println(" Firmware v0.0.1");
    Serial.println("===============================");
}

void Receiver::update()
{
    // Future code goes here
}