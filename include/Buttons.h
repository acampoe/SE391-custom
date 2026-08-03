#pragma once

#include <Arduino.h>
#include "ButtonsDatabase.h"

class Buttons
{
public:
    void begin();
    void update();

    // nullptr means that no known front-panel button is pressed.
    const ButtonSignature *getButton() const;
    const uint16_t *getAdcValues() const;
    bool hasPress() const;

private:
    static constexpr uint16_t SAMPLE_COUNT = 16;
    static constexpr uint16_t PRESS_DELTA = 35;
    static constexpr uint16_t MATCH_TOLERANCE = 100;
    static constexpr uint8_t DEBOUNCE_READS = 3;

    void readAverage(uint16_t values[SONY_ADC_COUNT]);
    const ButtonSignature *findNearest(
        const uint16_t values[SONY_ADC_COUNT]) const;
    bool isPressed(const uint16_t values[SONY_ADC_COUNT]) const;

    uint16_t idleValues[SONY_ADC_COUNT] = {};
    uint16_t adcValues[SONY_ADC_COUNT] = {};
    const ButtonSignature *candidateButton = nullptr;
    const ButtonSignature *currentButton = nullptr;
    bool candidatePressed = false;
    bool currentPressed = false;
    uint8_t candidateReads = 0;
};
