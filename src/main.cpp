#include <Arduino.h>
#include <Arduino.h>
#include "Receiver.h"

Receiver receiver;

void setup()
{
    receiver.begin();
}

void loop()
{
    receiver.update();
}