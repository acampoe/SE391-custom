#include <Arduino.h>

#include "Receiver.h"
#include "Config.h"
#include "Pins.h"

void Receiver::begin()
{
    logger.begin();

    // The KEY board supplies the 3.3 V pull-up. Releasing this open-drain
    // output keeps the active-low 5.1CH/DVD LED control safely off.
    digitalWrite(Pins::LED_FIVE_ONE_CH, HIGH);
    pinMode(Pins::LED_FIVE_ONE_CH, OUTPUT_OPEN_DRAIN);

    // Bass Boost is active high through the KEY-board's Q242 driver.
    digitalWrite(Pins::LED_BASS_BOOST, LOW);
    pinMode(Pins::LED_BASS_BOOST, OUTPUT);

    // Muting uses an active-low KEY-board driver like 5.1CH/DVD.
    digitalWrite(Pins::LED_MUTING, HIGH);
    pinMode(Pins::LED_MUTING, OUTPUT_OPEN_DRAIN);

    pinMode(Pins::BUTTON_POWER,
            Config::POWER_BUTTON_ACTIVE_LOW ? INPUT_PULLUP : INPUT_PULLDOWN);
    powerRawPressed =
        digitalRead(Pins::BUTTON_POWER) ==
        (Config::POWER_BUTTON_ACTIVE_LOW ? LOW : HIGH);
    powerStablePressed = powerRawPressed;
    powerLastChangeMs = millis();

    buttons.begin();
    inputSelector.begin();
    audio.begin();
    display.begin();

    logger.info("Receiver controller ready.");
    Serial.println("[STATE] STANDBY");
    updatePanelLeds();
}

void Receiver::update()
{
    updatePowerButton();
    buttons.update();

    const ButtonSignature *button = buttons.getButton();
    const bool buttonPressed = buttons.hasPress();

    // Act once on the physical press edge. A noisy ADC reading may change the
    // nearest signature while a key is held, but it must not retrigger an
    // action until the key has been released.
    if (buttonPressed != lastButtonPressed) {
        if (buttonPressed && poweredOn) {
            const uint16_t *adc = buttons.getAdcValues();

            if (button != nullptr) {
                Serial.print("[BUTTON] ");
                Serial.print(button->name);
                Serial.print("  ADC: ");
                for (uint8_t channel = 0; channel < SONY_ADC_COUNT; ++channel) {
                    Serial.print(adc[channel]);
                    if (channel + 1 < SONY_ADC_COUNT)
                        Serial.print(',');
                }
                for (uint8_t channel = 0; channel < SONY_ADC_COUNT; ++channel) {
                    const uint16_t lowRange = buttons.getLowRangeAdc(channel);
                    if (lowRange != UINT16_MAX) {
                        Serial.print("  LOW_ADC");
                        Serial.print(channel);
                        Serial.print(": ");
                        Serial.print(lowRange);
                    }
                }
                Serial.println();
                handleFrontPanelButton(button);
            } else {
                Serial.print("[BUTTON] UNKNOWN  ADC: ");
                for (uint8_t channel = 0; channel < SONY_ADC_COUNT; ++channel) {
                    Serial.print(adc[channel]);
                    if (channel + 1 < SONY_ADC_COUNT)
                        Serial.print(',');
                }
                Serial.println();
            }
        }

        lastButtonPressed = buttonPressed;
    }

    lastButton = button;

    if (poweredOn) {
        audio.update();
        display.update();
    }
}

void Receiver::handleFrontPanelButton(const ButtonSignature *button)
{
    if (strcmp(button->name, "VIDEO") == 0)
        selectInput(InputSource::Video);
    else if (strcmp(button->name, "TV_LD") == 0)
        selectInput(InputSource::TvLd);
    else if (strcmp(button->name, "MD_TAPE") == 0)
        selectInput(InputSource::MdTape);
    else if (strcmp(button->name, "FIVE_ONE_CH_DVD") == 0)
        selectInput(InputSource::DvdFiveOne);
    else if (strcmp(button->name, "CD") == 0)
        selectInput(InputSource::Cd);
    else if (strcmp(button->name, "TUNER") == 0)
        selectInput(InputSource::Tuner);
    else if (strcmp(button->name, "PHONO") == 0)
        selectInput(InputSource::Phono);
    else if (strcmp(button->name, "BASS_BOOST") == 0)
        toggleBassBoost();
    else if (strcmp(button->name, "MUTE") == 0)
        toggleMute();
}

void Receiver::updatePowerButton()
{
    const bool rawPressed =
        digitalRead(Pins::BUTTON_POWER) ==
        (Config::POWER_BUTTON_ACTIVE_LOW ? LOW : HIGH);

    if (rawPressed != powerRawPressed) {
        powerRawPressed = rawPressed;
        powerLastChangeMs = millis();
    }

    if (rawPressed == powerStablePressed ||
        millis() - powerLastChangeMs < Config::POWER_BUTTON_DEBOUNCE_MS) {
        return;
    }

    powerStablePressed = rawPressed;

    // Toggle only on the press edge; releasing the key produces no action.
    if (powerStablePressed) {
        setPoweredOn(!poweredOn);
    }
}

void Receiver::setPoweredOn(bool on)
{
    if (poweredOn == on)
        return;

    poweredOn = on;

    if (poweredOn) {
        applyInputSelection();
        Serial.println("[STATE] ON");
        Serial.print("[INPUT] ");
        Serial.println(inputName(selectedInput));
    } else {
        inputSelector.allOff();
        Serial.println("[STATE] STANDBY");
    }

    updatePanelLeds();
}

void Receiver::selectInput(InputSource input)
{
    if (selectedInput == input)
        return;

    selectedInput = input;
    applyInputSelection();
    Serial.print("[INPUT] ");
    Serial.println(inputName(selectedInput));
    updatePanelLeds();
}

void Receiver::applyInputSelection()
{
    if (!poweredOn) {
        inputSelector.allOff();
        return;
    }

    switch (selectedInput) {
    case InputSource::Phono:
        inputSelector.select(LC78212::Switch::Phono);
        break;
    case InputSource::Cd:
        inputSelector.select(LC78212::Switch::Cd);
        break;
    case InputSource::Tuner:
        inputSelector.select(LC78212::Switch::Tuner);
        break;
    case InputSource::Video:
        inputSelector.select(LC78212::Switch::Video);
        break;
    case InputSource::TvLd:
        inputSelector.select(LC78212::Switch::TvLd);
        break;
    case InputSource::MdTape:
        inputSelector.select(LC78212::Switch::MdTape);
        break;
    case InputSource::DvdFiveOne:
        // The 5.1/DVD path bypasses IC401 in the original receiver.
        inputSelector.allOff();
        break;
    }
}

void Receiver::toggleBassBoost()
{
    bassBoostOn = !bassBoostOn;
    Serial.print("[BASS BOOST] ");
    Serial.println(bassBoostOn ? "ON" : "OFF");
    updatePanelLeds();
}

void Receiver::toggleMute()
{
    muted = !muted;
    Serial.print("[MUTE] ");
    Serial.println(muted ? "ON" : "OFF");
    updatePanelLeds();
}

void Receiver::updatePanelLeds()
{
    const bool fiveOneSelected =
        poweredOn && selectedInput == InputSource::DvdFiveOne;

    digitalWrite(Pins::LED_FIVE_ONE_CH, fiveOneSelected ? LOW : HIGH);
    digitalWrite(Pins::LED_BASS_BOOST,
                 poweredOn && bassBoostOn ? HIGH : LOW);
    digitalWrite(Pins::LED_MUTING,
                 poweredOn && muted ? LOW : HIGH);
}

const char *Receiver::inputName(InputSource input)
{
    switch (input) {
    case InputSource::Video:       return "VIDEO";
    case InputSource::TvLd:        return "TV/LD";
    case InputSource::MdTape:      return "MD/TAPE";
    case InputSource::DvdFiveOne:  return "5.1CH/DVD";
    case InputSource::Cd:          return "CD";
    case InputSource::Tuner:       return "TUNER";
    case InputSource::Phono:       return "PHONO";
    }

    return "UNKNOWN";
}

bool Receiver::isPoweredOn() const
{
    return poweredOn;
}

Receiver::InputSource Receiver::getInputSource() const
{
    return selectedInput;
}
