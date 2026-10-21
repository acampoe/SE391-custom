#include <Arduino.h>

#include "Config.h"
#include "Buttons.h"
#include "LC78212.h"
#include "Pins.h"
#include "PT6312.h"

namespace {
PT6312 displayDriver(Pins::PT6312_DIN, Pins::PT6312_CLK, Pins::PT6312_STB);
LC78212 inputSelector;
Buttons keyboard;
constexpr char SCROLL_MESSAGE[] = "YOU ARE THE BEST";
constexpr uint32_t SCROLL_INTERVAL_MS = 350;
constexpr uint32_t DIAGNOSTIC_DISPLAY_MS = 2000;
bool receiverOn = false;
bool rawPressed = false;
bool stablePressed = false;
bool scrollingMessage = false;
bool keyboardPressLatched = false;
int16_t scrollOffset = -8;
uint32_t nextScrollMs = 0;
uint32_t lastRawChangeMs = 0;
uint32_t nextKeyboardDiagnosticMs = 0;
uint32_t diagnosticDisplayUntilMs = 0;
LC78212::Switch selectedInput = LC78212::Switch::Cd;
const char *selectedInputText = "CD";

void showSelectedInput()
{
    if (selectedInput == LC78212::Switch::Cd)
        displayDriver.showCd();
    else
        displayDriver.showTextWindow(selectedInputText, 0);
}

bool selectInputForButton(const char *name)
{
    if (strcmp(name, "CD") == 0) {
        selectedInput = LC78212::Switch::Cd;
        selectedInputText = "CD";
    } else if (strcmp(name, "TUNER") == 0) {
        selectedInput = LC78212::Switch::Tuner;
        selectedInputText = "TUNER";
    } else if (strcmp(name, "PHONO") == 0) {
        selectedInput = LC78212::Switch::Phono;
        selectedInputText = "PHONO";
    } else if (strcmp(name, "VIDEO") == 0) {
        selectedInput = LC78212::Switch::Video;
        selectedInputText = "VIDEO";
    } else if (strcmp(name, "TV_LD") == 0) {
        selectedInput = LC78212::Switch::TvLd;
        selectedInputText = "TV LD";
    } else if (strcmp(name, "MD_TAPE") == 0) {
        selectedInput = LC78212::Switch::MdTape;
        selectedInputText = "MD TAPE";
    } else {
        return false;
    }

    inputSelector.select(selectedInput);
    scrollingMessage = false;
    diagnosticDisplayUntilMs = 0;
    showSelectedInput();
    Serial.printf("[INPUT] %s (LC78212 switch %u)\n",
                  selectedInputText, static_cast<unsigned>(selectedInput));
    return true;
}

void showDiagnostic(const char *text)
{
    scrollingMessage = false;
    displayDriver.showTextWindow(text, 0);
    diagnosticDisplayUntilMs = millis() + DIAGNOSTIC_DISPLAY_MS;
}

const char *buttonDisplayText(const char *name)
{
    if (strcmp(name, "TUNING_MINUS") == 0)
        return "TUNE -";
    if (strcmp(name, "TUNING_PLUS") == 0)
        return "TUNE +";
    if (strcmp(name, "PRESET_MINUS") == 0)
        return "PRESET-";
    if (strcmp(name, "PRESET_PLUS") == 0)
        return "PRESET+";
    if (strcmp(name, "BASS_BOOST") == 0)
        return "BASS ON";
    if (strcmp(name, "MUTE") == 0)
        return "MUTE ON";
    if (strcmp(name, "MD_TAPE") == 0)
        return "MD TAPE";
    if (strcmp(name, "TV_LD") == 0)
        return "TV LD";
    if (strcmp(name, "FIVE_ONE_CH_DVD") == 0)
        return "DVD";
    if (strcmp(name, "DOLBY_MINUS") == 0)
        return "DOLBY -";
    if (strcmp(name, "DOLBY_MODE") == 0)
        return "DOLBY ON";
    if (strcmp(name, "DOLBY_PLUS") == 0)
        return "DOLBY +";
    if (strcmp(name, "FM_MODE") == 0)
        return "FM MODE";
    if (strcmp(name, "EFFECT_DELAY") == 0)
        return "EFFECT";
    if (strcmp(name, "VIDEO") == 0)
        return "VIDEO";
    if (strcmp(name, "TUNER") == 0)
        return "TUNER";
    if (strcmp(name, "PHONO") == 0)
        return "PHONO";
    if (strcmp(name, "CD") == 0)
        return "CD";
    return name;
}

bool readPowerButton()
{
    return digitalRead(Pins::BUTTON_POWER) == LOW;
}

void setMainPower(bool on)
{
    if (!on && receiverOn) {
        inputSelector.allOff();
        // Blank the high-voltage outputs while the PT6312 is still powered.
        displayDriver.setDisplayEnabled(false);
        delay(5);
    }

    receiverOn = on;
    if (!receiverOn) {
        scrollingMessage = false;
        keyboardPressLatched = false;
    }
    digitalWrite(Pins::RELAY_MAIN, receiverOn ? HIGH : LOW);
    Serial.println(receiverOn ? "[STATE] ON" : "[STATE] STANDBY");

    if (receiverOn) {
        // Allow the switched +5 V and VEE rails to settle before commands.
        delay(300);
        // The keyboard is powered by the switched receiver supply, so its
        // released-button baseline must be learned only after that rail is up.
        keyboard.begin();
        keyboardPressLatched = false;
        nextKeyboardDiagnosticMs = millis();
        Serial.println("[KEYBOARD] Baseline captured; ready");
        displayDriver.begin();
        Serial.println("[PT6312] Initialized in 8-grid/14-segment mode");
        displayDriver.showWelcome();
        Serial.println("[PT6312] Displaying: WELCOME");
        delay(700);
        inputSelector.begin();
        // Temporary hardware diagnostic: close every LC78212 switch. This
        // removes source-to-switch mapping from the test. Connect only one
        // audio source while this diagnostic build is installed.
        inputSelector.setMask(0xFF);
        Serial.println("[LC78212] DIAGNOSTIC: all eight switches ON");
        scrollingMessage = false;
        diagnosticDisplayUntilMs = 0;
        displayDriver.showTextWindow("ALL ON", 0);
        Serial.println("[PT6312] Displaying: ALL ON");
    }
}

void processKeyboard()
{
    if (!receiverOn)
        return;

    keyboard.update();

    if (static_cast<int32_t>(millis() - nextKeyboardDiagnosticMs) >= 0) {
        const uint16_t *diagnosticAdc = keyboard.getAdcValues();
        Serial.printf("[KEY ADC] %u,%u,%u,%u\n",
                      diagnosticAdc[0], diagnosticAdc[1],
                      diagnosticAdc[2], diagnosticAdc[3]);
        nextKeyboardDiagnosticMs = millis() + 500;
    }

    if (!keyboard.hasPress()) {
        keyboardPressLatched = false;
        return;
    }

    if (keyboardPressLatched)
        return;

    keyboardPressLatched = true;
    const ButtonSignature *button = keyboard.getButton();
    const uint16_t *adc = keyboard.getAdcValues();

    if (button) {
        Serial.printf("[BUTTON] %s  ADC: %u,%u,%u,%u\n",
                      button->name, adc[0], adc[1], adc[2], adc[3]);

        if (!selectInputForButton(button->name)) {
            showDiagnostic(buttonDisplayText(button->name));
        }
    } else {
        Serial.printf("[BUTTON] UNKNOWN  ADC: %u,%u,%u,%u\n",
                      adc[0], adc[1], adc[2], adc[3]);

        // Show the channel with the lowest reading; a pressed ladder key pulls
        // one channel down from its released value near 3000.
        uint8_t changedChannel = 0;
        for (uint8_t channel = 1; channel < SONY_ADC_COUNT; ++channel) {
            if (adc[channel] < adc[changedChannel])
                changedChannel = channel;
        }

        char diagnostic[9];
        snprintf(diagnostic, sizeof(diagnostic), "A%u %04u",
                 changedChannel + 1, adc[changedChannel]);
        showDiagnostic(diagnostic);
    }
}
}

void setup()
{
    Serial.begin(Config::SERIAL_BAUD);
    delay(250);

    // Force the relay transistor off before enabling the GPIO output driver.
    digitalWrite(Pins::RELAY_MAIN, LOW);
    pinMode(Pins::RELAY_MAIN, OUTPUT);

    // The power switch shorts GPIO27 to GND when pressed.
    pinMode(Pins::BUTTON_POWER, INPUT_PULLUP);
    rawPressed = readPowerButton();
    stablePressed = rawPressed;
    lastRawChangeMs = millis();

    Serial.println("\n========================================");
    Serial.println(" Sony STR-SE391 Power Controller");
    Serial.println(" Clean GPIO baseline");
    Serial.println("========================================");
    Serial.println("[INFO] Power button: GPIO27, active LOW");
    Serial.println("[INFO] Main relay: GPIO17, active HIGH");
    Serial.println("[INFO] PT6312: DIN=18 CLK=19 STB=23");
    Serial.println("[INFO] LC78212: CE=21 DI=22 CL=25 (inverted NPN interface)");
    Serial.println("[INFO] Keyboard ADC: GPIO32, GPIO33, GPIO34, GPIO35");
    Serial.println("[INFO] Release all keyboard buttons while powering on");
    setMainPower(false);
}

void loop()
{
    processKeyboard();

    const bool pressed = readPowerButton();

    if (pressed != rawPressed) {
        rawPressed = pressed;
        lastRawChangeMs = millis();
    }

    if (pressed != stablePressed &&
        millis() - lastRawChangeMs >= Config::POWER_BUTTON_DEBOUNCE_MS) {
        stablePressed = pressed;

        // Toggle only on the press edge. Release does nothing.
        if (stablePressed)
            setMainPower(!receiverOn);
    }

    if (receiverOn && scrollingMessage &&
        static_cast<int32_t>(millis() - nextScrollMs) >= 0) {
        ++scrollOffset;
        nextScrollMs += SCROLL_INTERVAL_MS;

        if (scrollOffset >= static_cast<int16_t>(strlen(SCROLL_MESSAGE))) {
            scrollingMessage = false;
            showSelectedInput();
            Serial.printf("[PT6312] Displaying: %s\n", selectedInputText);
        } else {
            displayDriver.showTextWindow(SCROLL_MESSAGE, scrollOffset);
        }
    }

    if (receiverOn && diagnosticDisplayUntilMs != 0 &&
        static_cast<int32_t>(millis() - diagnosticDisplayUntilMs) >= 0) {
        diagnosticDisplayUntilMs = 0;
        showSelectedInput();
    }

    delay(1);
}
