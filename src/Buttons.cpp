#include "Buttons.h"

namespace
{
constexpr uint8_t ADC_PINS[SONY_ADC_COUNT] = {32, 33, 34, 35};

// No-button values present when sonyButtonDB was captured. Runtime signatures
// are scaled to the idle values learned at boot, allowing for supply/divider
// variation without changing the relative ladder ratios.
constexpr uint16_t CALIBRATION_IDLE[SONY_ADC_COUNT] =
    {2940, 2945, 2960, 2958};
}

void Buttons::begin()
{
    candidateButton = nullptr;
    currentButton = nullptr;
    candidatePressed = false;
    currentPressed = false;
    candidateSinceMs = 0;

    analogReadResolution(12);

    for (uint8_t channel = 0; channel < SONY_ADC_COUNT; ++channel) {
        pinMode(ADC_PINS[channel], INPUT);
        analogSetPinAttenuation(ADC_PINS[channel], ADC_11db);
    }

    // begin() must be called while all front-panel buttons are released.
    delay(100);
    readAverage(idleValues);
}

uint16_t Buttons::readLowRange(uint8_t channel)
{
    analogSetPinAttenuation(ADC_PINS[channel], ADC_0db);
    delayMicroseconds(100);

    uint32_t sum = 0;
    for (uint16_t sample = 0; sample < SAMPLE_COUNT; ++sample) {
        sum += analogRead(ADC_PINS[channel]);
        delayMicroseconds(250);
    }

    analogSetPinAttenuation(ADC_PINS[channel], ADC_11db);
    return static_cast<uint16_t>(sum / SAMPLE_COUNT);
}

void Buttons::readAverage(uint16_t values[SONY_ADC_COUNT])
{
    uint32_t sums[SONY_ADC_COUNT] = {};

    for (uint16_t sample = 0; sample < SAMPLE_COUNT; ++sample) {
        for (uint8_t channel = 0; channel < SONY_ADC_COUNT; ++channel) {
            sums[channel] += analogRead(ADC_PINS[channel]);
        }
        delay(1);
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
    // The ADC has compressed/nonlinear behavior near zero. These adjacent
    // ladder positions therefore use measured, non-overlapping bands instead
    // of the generic +/- tolerance used by the rest of the database.
    if (values[1] <= 110) {
        return findByName(lowRangeAdc[1] >= LOW_RANGE_SPLIT
                              ? "MUTE"
                              : "BASS_BOOST");
    }

    if (values[0] <= LOW_RANGE_TRIGGER) {
        return findByName(lowRangeAdc[0] >= LOW_RANGE_SPLIT
                              ? "NAME"
                              : "SETUP");
    }

    if (values[2] <= LOW_RANGE_TRIGGER) {
        return findByName(lowRangeAdc[2] >= LOW_RANGE_SPLIT
                              ? "6"
                              : "FM");
    }

    if (values[0] >= 110 && values[0] <= 330)
        return findByName("ENTER");

    // Only one ladder channel is selected by a normal key. Match primarily
    // on the channel with the largest proportional drop; inactive channels
    // can move with supply loading and must not reject an otherwise valid key.
    uint8_t activeChannel = 0;
    uint32_t largestDrop = 0;
    for (uint8_t channel = 0; channel < SONY_ADC_COUNT; ++channel) {
        const uint32_t drop = values[channel] < idleValues[channel]
                                  ? static_cast<uint32_t>(idleValues[channel] - values[channel])
                                  : 0;
        if (drop > largestDrop) {
            largestDrop = drop;
            activeChannel = channel;
        }
    }

    const ButtonSignature *nearest = nullptr;
    uint32_t nearestDistance = UINT32_MAX;

    for (size_t button = 0; button < SONY_BUTTON_COUNT; ++button) {
        uint8_t expectedActiveChannel = 0;
        uint16_t expectedLargestDrop = 0;
        for (uint8_t channel = 0; channel < SONY_ADC_COUNT; ++channel) {
            const uint16_t expectedDrop =
                sonyButtonDB[button].adc[channel] < CALIBRATION_IDLE[channel]
                    ? CALIBRATION_IDLE[channel] - sonyButtonDB[button].adc[channel]
                    : 0;
            if (expectedDrop > expectedLargestDrop) {
                expectedLargestDrop = expectedDrop;
                expectedActiveChannel = channel;
            }
        }

        if (expectedActiveChannel != activeChannel)
            continue;

        const uint16_t expected = static_cast<uint16_t>(
            (static_cast<uint32_t>(sonyButtonDB[button].adc[activeChannel]) *
             idleValues[activeChannel] + CALIBRATION_IDLE[activeChannel] / 2) /
            CALIBRATION_IDLE[activeChannel]);
        const uint16_t distance = abs(
            static_cast<int>(values[activeChannel]) -
            static_cast<int>(expected));

        if (distance <= MATCH_TOLERANCE && distance < nearestDistance) {
            nearestDistance = distance;
            nearest = &sonyButtonDB[button];
        }
    }

    return nearest;
}

const ButtonSignature *Buttons::findByName(const char *name) const
{
    for (size_t button = 0; button < SONY_BUTTON_COUNT; ++button) {
        if (strcmp(sonyButtonDB[button].name, name) == 0)
            return &sonyButtonDB[button];
    }

    return nullptr;
}

void Buttons::update()
{
    readAverage(adcValues);

    for (uint8_t channel = 0; channel < SONY_ADC_COUNT; ++channel) {
        lowRangeAdc[channel] =
            adcValues[channel] <= LOW_RANGE_TRIGGER
                ? readLowRange(channel)
                : UINT16_MAX;
    }

    const bool observedPressed = isPressed(adcValues);
    const ButtonSignature *observed =
        observedPressed ? findNearest(adcValues) : nullptr;

    if (observedPressed != candidatePressed || observed != candidateButton) {
        candidatePressed = observedPressed;
        candidateButton = observed;
        candidateSinceMs = millis();
        return;
    }

    if (millis() - candidateSinceMs >= SETTLE_MS) {
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

uint16_t Buttons::getLowRangeAdc(uint8_t channel) const
{
    return channel < SONY_ADC_COUNT ? lowRangeAdc[channel] : UINT16_MAX;
}

bool Buttons::hasPress() const
{
    return currentPressed;
}
