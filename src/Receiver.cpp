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

    pinMode(Pins::BUTTON_POWER,
            Config::POWER_BUTTON_ACTIVE_LOW ? INPUT_PULLUP : INPUT_PULLDOWN);
    powerRawPressed =
        digitalRead(Pins::BUTTON_POWER) ==
        (Config::POWER_BUTTON_ACTIVE_LOW ? LOW : HIGH);
    powerStablePressed = powerRawPressed;
    powerLastChangeMs = millis();

    buttons.begin();
    audio.begin();
    display.begin();

    logger.info("Receiver controller ready.");
    Serial.println("[STATE] STANDBY");
    updateInputLed();
}

void Receiver::update()
{
    updatePowerButton();
    buttons.update();

    const ButtonSignature *button = buttons.getButton();
    const bool buttonPressed = buttons.hasPress();

    // Act only on a new, debounced press edge.
    if (buttonPressed != lastButtonPressed || button != lastButton) {
        if (buttonPressed && button != nullptr && poweredOn) {
            handleFrontPanelButton(button);
        }

        lastButton = button;
        lastButtonPressed = buttonPressed;
    }

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
        Serial.println("[STATE] ON");
        Serial.print("[INPUT] ");
        Serial.println(inputName(selectedInput));
    } else {
        Serial.println("[STATE] STANDBY");
    }

    updateInputLed();
}

void Receiver::selectInput(InputSource input)
{
    if (selectedInput == input)
        return;

    selectedInput = input;
    Serial.print("[INPUT] ");
    Serial.println(inputName(selectedInput));
    updateInputLed();
}

void Receiver::updateInputLed()
{
    const bool fiveOneSelected =
        poweredOn && selectedInput == InputSource::DvdFiveOne;

    // Active low: LOW sinks the KEY-board control line and lights D239.
    digitalWrite(Pins::LED_FIVE_ONE_CH, fiveOneSelected ? LOW : HIGH);
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
