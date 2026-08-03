#pragma once

#include "Buttons.h"
#include "Display.h"
#include "Audio.h"
#include "Remote.h"
#include "Menu.h"
#include "Settings.h"
#include "Logger.h"

class Receiver
{
public:
    enum class InputSource
    {
        Video,
        TvLd,
        MdTape,
        DvdFiveOne,
        Cd,
        Tuner,
        Phono
    };

    void begin();
    void update();
    bool isPoweredOn() const;
    InputSource getInputSource() const;

private:
    void updatePowerButton();
    void setPoweredOn(bool on);
    void handleFrontPanelButton(const ButtonSignature *button);
    void selectInput(InputSource input);
    void updateInputLed();
    static const char *inputName(InputSource input);

    const ButtonSignature *lastButton = nullptr;
    bool lastButtonPressed = false;

    bool poweredOn = false;
    bool powerRawPressed = false;
    bool powerStablePressed = false;
    uint32_t powerLastChangeMs = 0;
    InputSource selectedInput = InputSource::Cd;

    Logger logger;
    Buttons buttons;
    Display display;
    Audio audio;
    Remote remote;
    Menu menu;
    Settings settings;
};
