#pragma once

#include <Arduino.h>

constexpr uint8_t SONY_ADC_COUNT = 4;

struct ButtonSignature
{
    const char *name;
    uint16_t adc[SONY_ADC_COUNT];
};

extern const ButtonSignature sonyButtonDB[];
extern const size_t SONY_BUTTON_COUNT;
