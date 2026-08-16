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
    uint16_t getLowRangeAdc(uint8_t channel) const;
    bool hasPress() const;

private:
    static constexpr uint16_t SAMPLE_COUNT = 32;
    static constexpr uint16_t PRESS_DELTA = 35;
    static constexpr uint16_t MATCH_TOLERANCE = 100;
    static constexpr uint16_t SETTLE_MS = 60;
    static constexpr uint16_t LOW_RANGE_TRIGGER = 110;
    static constexpr uint16_t LOW_RANGE_SPLIT = 100;

    void readAverage(uint16_t values[SONY_ADC_COUNT]);
    uint16_t readLowRange(uint8_t channel);
    const ButtonSignature *findNearest(
        const uint16_t values[SONY_ADC_COUNT]) const;
    const ButtonSignature *findByName(const char *name) const;
    bool isPressed(const uint16_t values[SONY_ADC_COUNT]) const;

    uint16_t idleValues[SONY_ADC_COUNT] = {};
    uint16_t adcValues[SONY_ADC_COUNT] = {};
    uint16_t lowRangeAdc[SONY_ADC_COUNT] =
        {UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX};
    const ButtonSignature *candidateButton = nullptr;
    const ButtonSignature *currentButton = nullptr;
    bool candidatePressed = false;
    bool currentPressed = false;
    uint32_t candidateSinceMs = 0;
};
