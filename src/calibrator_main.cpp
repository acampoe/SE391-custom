#include <Arduino.h>

namespace {
constexpr uint8_t ADC_PINS[] = {32, 33, 26, 27};
constexpr size_t ADC_COUNT = sizeof(ADC_PINS) / sizeof(ADC_PINS[0]);
constexpr uint16_t PRESS_DELTA = 100;
constexpr uint16_t RELEASE_DELTA = 60;

const char* const BUTTON_NAMES[] = {
    "TUNING_MINUS", "TUNING_PLUS", "PRESET_MINUS", "PRESET_PLUS",
    "FM", "AM", "VIDEO", "TV_LD", "MD_TAPE", "FIVE_ONE_CH_DVD",
    "DOLBY_MINUS", "DOLBY_MODE", "DOLBY_PLUS", "SETUP", "BASS_BOOST",
    "SHIFT", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
    "DIRECT", "MEMORY", "FM_MODE", "CD", "TUNER", "PHONO",
    "EFFECT_DELAY", "CENTER", "MINUS", "PLUS", "REAR", "ENTER", "NAME",
    "MUTE"
};

constexpr size_t BUTTON_COUNT =
    sizeof(BUTTON_NAMES) / sizeof(BUTTON_NAMES[0]);

uint16_t idle[ADC_COUNT] = {};
uint16_t captured[BUTTON_COUNT][ADC_COUNT] = {};

void readAveraged(uint16_t values[ADC_COUNT], uint16_t samples = 32)
{
    uint32_t sums[ADC_COUNT] = {};
    for (uint16_t sample = 0; sample < samples; ++sample) {
        for (size_t channel = 0; channel < ADC_COUNT; ++channel)
            sums[channel] += analogRead(ADC_PINS[channel]);
        delay(2);
    }

    for (size_t channel = 0; channel < ADC_COUNT; ++channel)
        values[channel] = sums[channel] / samples;
}

bool differsFromIdle(const uint16_t values[ADC_COUNT], uint16_t threshold)
{
    for (size_t channel = 0; channel < ADC_COUNT; ++channel) {
        if (abs(static_cast<int>(values[channel]) -
                static_cast<int>(idle[channel])) >= threshold)
            return true;
    }
    return false;
}

void printValues(const uint16_t values[ADC_COUNT])
{
    Serial.printf("%u,%u,%u,%u", values[0], values[1], values[2], values[3]);
}

void printDatabase()
{
    Serial.println("\n\n========================================");
    Serial.println(" CALIBRATION COMPLETE");
    Serial.println("========================================\n");
    Serial.println("Copy the database below:\n");
    Serial.println("struct ButtonSignature");
    Serial.println("{");
    Serial.println("    const char *name;");
    Serial.println("    uint16_t adc[4];");
    Serial.println("};\n");
    Serial.println("const ButtonSignature sonyButtonDB[] =");
    Serial.println("{");
    for (size_t button = 0; button < BUTTON_COUNT; ++button) {
        Serial.printf("    {\"%s\",{", BUTTON_NAMES[button]);
        printValues(captured[button]);
        Serial.println("}},");
    }
    Serial.println("};\n");
    Serial.printf("const size_t SONY_BUTTON_COUNT = %u;\n\n",
                  static_cast<unsigned>(BUTTON_COUNT));
    Serial.println("End of database.");
}
}

void setup()
{
    Serial.begin(9600);
    delay(1500);

    analogReadResolution(12);
    for (size_t channel = 0; channel < ADC_COUNT; ++channel) {
        pinMode(ADC_PINS[channel], INPUT);
        analogSetPinAttenuation(ADC_PINS[channel], ADC_11db);
    }

    Serial.println("\n========================================");
    Serial.println(" SONY KEYBOARD CALIBRATOR");
    Serial.println("========================================");
    Serial.println("Release every button. Measuring idle values...");
    delay(1500);
    readAveraged(idle, 64);
    Serial.print("Idle ADC: ");
    printValues(idle);
    Serial.println("\n");

    for (size_t button = 0; button < BUTTON_COUNT; ++button) {
        Serial.println("----------------------------------------");
        Serial.printf("Button %u of %u\n",
                      static_cast<unsigned>(button + 1),
                      static_cast<unsigned>(BUTTON_COUNT));
        Serial.printf("PRESS AND HOLD: %s\n", BUTTON_NAMES[button]);
        Serial.println("----------------------------------------");

        uint16_t values[ADC_COUNT];
        do {
            readAveraged(values, 4);
        } while (!differsFromIdle(values, PRESS_DELTA));

        delay(100);
        readAveraged(captured[button], 64);
        Serial.printf("Captured %s: ", BUTTON_NAMES[button]);
        printValues(captured[button]);
        Serial.println();
        Serial.println("Release the button...");

        do {
            readAveraged(values, 4);
        } while (differsFromIdle(values, RELEASE_DELTA));

        Serial.println("Released.\n");
        delay(150);
    }

    printDatabase();
}

void loop()
{
    delay(1000);
}
