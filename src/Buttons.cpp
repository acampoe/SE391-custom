#include "Buttons.h"

namespace
{
constexpr uint8_t ADC_PINS[SONY_ADC_COUNT] = {32, 33, 26, 27};
}

void Buttons::begin()
{
    analogReadResolution(12);

    for (uint8_t channel = 0; channel < SONY_ADC_COUNT; ++channel) {
        pinMode(ADC_PINS[channel], INPUT);
    }

    // begin() must be called while all front-panel buttons are released.
    delay(100);
    readAverage(idleValues);
}

void Buttons::readAverage(uint16_t values[SONY_ADC_COUNT])
{
    uint32_t sums[SONY_ADC_COUNT] = {};

    for (uint16_t sample = 0; sample < SAMPLE_COUNT; ++sample) {
        for (uint8_t channel = 0; channel < SONY_ADC_COUNT; ++channel) {
            sums[channel] += analogRead(ADC_PINS[channel]);
        }
        delayMicroseconds(250);
    }

    for (uint8_t channel = 0; channel < SONY_ADC_COUNT; ++channel) {
        values[channel] = sums[channel] / SAMPLE_COUNT;
    }
}

bool Buttons::isPressed(const uint16_t values[SONY_ADC_COUNT]) const
{
    for (uint8_t channel = 0; channel < SONY_ADC_COUNT; ++channel) {
        const uint16_t difference = abs(
            static_cast<int>(values[channel]) -
            static_cast<int>(idleValues[channel]));

        if (difference >= PRESS_DELTA) {
            return true;
        }
    }

    return false;
}

const ButtonSignature *Buttons::findNearest(
    const uint16_t values[SONY_ADC_COUNT]) const
{
    const ButtonSignature *nearest = nullptr;
    uint32_t nearestDistance = UINT32_MAX;

    for (size_t button = 0; button < SONY_BUTTON_COUNT; ++button) {
        uint32_t distance = 0;
        bool withinTolerance = true;

        for (uint8_t channel = 0; channel < SONY_ADC_COUNT; ++channel) {
            const uint16_t difference = abs(
                static_cast<int>(values[channel]) -
                static_cast<int>(sonyButtonDB[button].adc[channel]));

            if (difference > MATCH_TOLERANCE) {
                withinTolerance = false;
                break;
            }
            distance += difference;
        }

        if (withinTolerance && distance < nearestDistance) {
            nearestDistance = distance;
            nearest = &sonyButtonDB[button];
        }
    }

    return nearest;
}

void Buttons::update()
{
    readAverage(adcValues);

    const bool observedPressed = isPressed(adcValues);
    const ButtonSignature *observed =
        observedPressed ? findNearest(adcValues) : nullptr;

    if (observedPressed != candidatePressed || observed != candidateButton) {
        candidatePressed = observedPressed;
        candidateButton = observed;
        candidateReads = 1;
        return;
    }

    if (candidateReads < DEBOUNCE_READS) {
        ++candidateReads;
    }

    if (candidateReads >= DEBOUNCE_READS) {
        currentPressed = candidatePressed;
        currentButton = candidateButton;
    }
}

const ButtonSignature *Buttons::getButton() const
{
    return currentButton;
}

const uint16_t *Buttons::getAdcValues() const
{
    return adcValues;
}

bool Buttons::hasPress() const
{
    return currentPressed;
}
