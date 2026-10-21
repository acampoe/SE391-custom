#include "PT6312.h"

namespace {
constexpr uint8_t COMMAND_MODE_8_GRIDS_14_SEGMENTS = 0x04;
constexpr uint8_t COMMAND_WRITE_AUTO_INCREMENT = 0x40;
constexpr uint8_t COMMAND_ADDRESS_ZERO = 0xC0;
constexpr uint8_t COMMAND_DISPLAY_OFF = 0x80;
constexpr uint8_t COMMAND_DISPLAY_ON = 0x88;
constexpr size_t DISPLAY_RAM_BYTES = 22;

constexpr uint16_t segment(uint8_t number)
{
    return static_cast<uint16_t>(1U << (number - 1));
}

uint16_t glyphFor(char character)
{
    constexpr uint16_t TOP = segment(1);
    constexpr uint16_t UPPER_LEFT = segment(6);
    constexpr uint16_t UPPER_RIGHT = segment(2);
    constexpr uint16_t MIDDLE_LEFT = segment(7);
    constexpr uint16_t MIDDLE_RIGHT = segment(11);
    constexpr uint16_t LOWER_LEFT = segment(5);
    constexpr uint16_t LOWER_RIGHT = segment(3);
    constexpr uint16_t BOTTOM = segment(4);
    constexpr uint16_t UPPER_LEFT_DIAGONAL = segment(8);
    constexpr uint16_t UPPER_CENTER = segment(9);
    constexpr uint16_t UPPER_RIGHT_DIAGONAL = segment(10);
    constexpr uint16_t LOWER_RIGHT_DIAGONAL = segment(12);
    constexpr uint16_t LOWER_CENTER = segment(13);
    constexpr uint16_t LOWER_LEFT_DIAGONAL = segment(14);

    switch (character) {
    case '0': return TOP | UPPER_LEFT | UPPER_RIGHT | LOWER_LEFT |
                     LOWER_RIGHT | BOTTOM;
    case '1': return UPPER_RIGHT | LOWER_RIGHT;
    case '2': return TOP | UPPER_RIGHT | MIDDLE_LEFT | MIDDLE_RIGHT |
                     LOWER_LEFT | BOTTOM;
    case '3': return TOP | UPPER_RIGHT | MIDDLE_LEFT | MIDDLE_RIGHT |
                     LOWER_RIGHT | BOTTOM;
    case '4': return UPPER_LEFT | UPPER_RIGHT | MIDDLE_LEFT |
                     MIDDLE_RIGHT | LOWER_RIGHT;
    case '5': return TOP | UPPER_LEFT | MIDDLE_LEFT | MIDDLE_RIGHT |
                     LOWER_RIGHT | BOTTOM;
    case '6': return TOP | UPPER_LEFT | MIDDLE_LEFT | MIDDLE_RIGHT |
                     LOWER_LEFT | LOWER_RIGHT | BOTTOM;
    case '7': return TOP | UPPER_RIGHT | LOWER_RIGHT;
    case '8': return TOP | UPPER_LEFT | UPPER_RIGHT | MIDDLE_LEFT |
                     MIDDLE_RIGHT | LOWER_LEFT | LOWER_RIGHT | BOTTOM;
    case '9': return TOP | UPPER_LEFT | UPPER_RIGHT | MIDDLE_LEFT |
                     MIDDLE_RIGHT | LOWER_RIGHT | BOTTOM;
    case 'A': return TOP | UPPER_LEFT | UPPER_RIGHT | MIDDLE_LEFT |
                     MIDDLE_RIGHT | LOWER_LEFT | LOWER_RIGHT;
    case 'B': return UPPER_LEFT | LOWER_LEFT | TOP | MIDDLE_LEFT |
                     MIDDLE_RIGHT | UPPER_RIGHT | LOWER_RIGHT | BOTTOM;
    case 'C': return TOP | UPPER_LEFT | LOWER_LEFT | BOTTOM;
    case 'D': return TOP | UPPER_RIGHT | LOWER_RIGHT | BOTTOM |
                     UPPER_CENTER | LOWER_CENTER;
    case 'E': return TOP | UPPER_LEFT | MIDDLE_LEFT | MIDDLE_RIGHT |
                     LOWER_LEFT | BOTTOM;
    case 'F': return TOP | UPPER_LEFT | MIDDLE_LEFT | MIDDLE_RIGHT |
                     LOWER_LEFT;
    case 'G': return TOP | UPPER_LEFT | MIDDLE_RIGHT | LOWER_LEFT |
                     LOWER_RIGHT | BOTTOM;
    case 'H': return UPPER_LEFT | UPPER_RIGHT | MIDDLE_LEFT |
                     MIDDLE_RIGHT | LOWER_LEFT | LOWER_RIGHT;
    case 'I': return TOP | UPPER_CENTER | LOWER_CENTER | BOTTOM;
    case 'L': return UPPER_LEFT | LOWER_LEFT | BOTTOM;
    case 'M': return UPPER_LEFT | UPPER_RIGHT | LOWER_LEFT | LOWER_RIGHT |
                     UPPER_LEFT_DIAGONAL | UPPER_RIGHT_DIAGONAL;
    case 'N': return UPPER_LEFT | UPPER_RIGHT | LOWER_LEFT | LOWER_RIGHT |
                     UPPER_LEFT_DIAGONAL | LOWER_RIGHT_DIAGONAL;
    case 'O': return TOP | UPPER_LEFT | UPPER_RIGHT | LOWER_LEFT |
                     LOWER_RIGHT | BOTTOM;
    case 'P': return TOP | UPPER_LEFT | UPPER_RIGHT | MIDDLE_LEFT |
                     MIDDLE_RIGHT | LOWER_LEFT;
    case 'R': return TOP | UPPER_LEFT | UPPER_RIGHT | MIDDLE_LEFT |
                     MIDDLE_RIGHT | LOWER_LEFT | LOWER_RIGHT_DIAGONAL;
    case 'S': return TOP | UPPER_LEFT | MIDDLE_LEFT | MIDDLE_RIGHT |
                     LOWER_RIGHT | BOTTOM;
    case 'T': return TOP | UPPER_CENTER | LOWER_CENTER;
    case 'U': return UPPER_LEFT | UPPER_RIGHT | LOWER_LEFT |
                     LOWER_RIGHT | BOTTOM;
    // Sony FL201-specific V: S6 + S5 + S14 + S10.
    case 'V': return segment(6) | segment(5) | segment(14) | segment(10);
    case 'Y': return UPPER_LEFT_DIAGONAL | UPPER_RIGHT_DIAGONAL |
                     LOWER_CENTER;
    case 'W': return UPPER_LEFT | UPPER_RIGHT | LOWER_LEFT | LOWER_RIGHT |
                     LOWER_LEFT_DIAGONAL | LOWER_RIGHT_DIAGONAL;
    case '-': return MIDDLE_LEFT | MIDDLE_RIGHT;
    case '+': return MIDDLE_LEFT | MIDDLE_RIGHT | UPPER_CENTER | LOWER_CENTER;
    case ' ': return 0;
    default: return 0;
    }
}
}

PT6312::PT6312(uint8_t dinPin, uint8_t clockPin, uint8_t strobePin)
    : dinPin_(dinPin), clockPin_(clockPin), strobePin_(strobePin)
{
}

void PT6312::begin()
{
    // Establish the inactive bus state before enabling the output drivers.
    digitalWrite(dinPin_, LOW);
    digitalWrite(clockPin_, LOW);
    digitalWrite(strobePin_, HIGH);
    pinMode(dinPin_, OUTPUT);
    pinMode(clockPin_, OUTPUT);
    pinMode(strobePin_, OUTPUT);

    // The datasheet recommends allowing the oscillator and supplies to settle.
    delay(200);

    sendCommand(COMMAND_DISPLAY_OFF);
    sendCommand(COMMAND_MODE_8_GRIDS_14_SEGMENTS);
    clear();
    sendCommand(COMMAND_DISPLAY_OFF);
}

void PT6312::clear()
{
    sendCommand(COMMAND_WRITE_AUTO_INCREMENT);

    digitalWrite(strobePin_, LOW);
    writeByteLsbFirst(COMMAND_ADDRESS_ZERO);
    for (size_t index = 0; index < DISPLAY_RAM_BYTES; ++index)
        writeByteLsbFirst(0x00);
    digitalWrite(strobePin_, HIGH);
}

void PT6312::setDisplayEnabled(bool enabled, uint8_t brightness)
{
    brightness &= 0x07;
    sendCommand(enabled ? static_cast<uint8_t>(COMMAND_DISPLAY_ON | brightness)
                        : COMMAND_DISPLAY_OFF);
}

void PT6312::showFirstIntersection()
{
    // For the initial hardware test only GR1 and SG1 are connected to
    // the tube. Fill RAM so those outputs are guaranteed active even if
    // a clone uses a slightly different display-memory interpretation.
    sendCommand(COMMAND_WRITE_AUTO_INCREMENT);

    digitalWrite(strobePin_, LOW);
    writeByteLsbFirst(COMMAND_ADDRESS_ZERO);
    for (size_t index = 0; index < DISPLAY_RAM_BYTES; ++index)
        writeByteLsbFirst(0xFF);
    digitalWrite(strobePin_, HIGH);

    // Maximum brightness (14/16 pulse width) for an unmistakable test.
    setDisplayEnabled(true, 7);
}

void PT6312::showIntersection(uint8_t grid, uint8_t segment)
{
    if (grid < 1 || grid > 8 || segment < 1 || segment > 14)
        return;

    uint8_t ram[DISPLAY_RAM_BYTES] = {};
    const size_t address = static_cast<size_t>(grid - 1) * 2U;
    if (segment <= 8)
        ram[address] = static_cast<uint8_t>(1U << (segment - 1));
    else
        ram[address + 1] = static_cast<uint8_t>(1U << (segment - 9));

    sendCommand(COMMAND_WRITE_AUTO_INCREMENT);
    digitalWrite(strobePin_, LOW);
    writeByteLsbFirst(COMMAND_ADDRESS_ZERO);
    for (size_t index = 0; index < DISPLAY_RAM_BYTES; ++index)
        writeByteLsbFirst(ram[index]);
    digitalWrite(strobePin_, HIGH);
    setDisplayEnabled(true, 7);
}

void PT6312::showChatGpt()
{
    // Sony FL201 stroke map supplied from the service-manual drawing.
    // S1-S14 are available in 8x14 mode. Only S15 is unavailable.
    constexpr uint16_t TOP = segment(1);
    constexpr uint16_t UPPER_LEFT = segment(6);
    constexpr uint16_t UPPER_RIGHT = segment(2);
    constexpr uint16_t MIDDLE_LEFT = segment(7);
    constexpr uint16_t MIDDLE_RIGHT = segment(11);
    constexpr uint16_t LOWER_LEFT = segment(5);
    constexpr uint16_t LOWER_RIGHT = segment(3);
    constexpr uint16_t BOTTOM = segment(4);
    constexpr uint16_t UPPER_CENTER = segment(9);
    constexpr uint16_t LOWER_CENTER = segment(13);

    constexpr uint16_t C = TOP | UPPER_LEFT | LOWER_LEFT | BOTTOM;
    constexpr uint16_t H = UPPER_LEFT | UPPER_RIGHT | MIDDLE_LEFT |
                           MIDDLE_RIGHT | LOWER_LEFT | LOWER_RIGHT;
    constexpr uint16_t A = TOP | UPPER_LEFT | UPPER_RIGHT | MIDDLE_LEFT |
                           MIDDLE_RIGHT | LOWER_LEFT | LOWER_RIGHT;
    constexpr uint16_t T = TOP | UPPER_CENTER | LOWER_CENTER;
    constexpr uint16_t G = C | MIDDLE_RIGHT | LOWER_RIGHT;
    constexpr uint16_t P = TOP | UPPER_LEFT | UPPER_RIGHT | MIDDLE_LEFT |
                           MIDDLE_RIGHT | LOWER_LEFT;

    // GR8 is the leftmost character (Sony DIG10); GR1 is the rightmost
    // character (Sony DIG3). This exactly covers the eight text positions.
    uint16_t grids[8] = {};
    grids[7] = C;
    grids[6] = H;
    grids[5] = A;
    grids[4] = T;
    grids[3] = 0;
    grids[2] = G;
    grids[1] = P;
    grids[0] = T;

    showGridPatterns(grids);
}

void PT6312::showWelcome()
{
    constexpr uint16_t TOP = segment(1);
    constexpr uint16_t UPPER_LEFT = segment(6);
    constexpr uint16_t UPPER_RIGHT = segment(2);
    constexpr uint16_t MIDDLE_LEFT = segment(7);
    constexpr uint16_t MIDDLE_RIGHT = segment(11);
    constexpr uint16_t LOWER_LEFT = segment(5);
    constexpr uint16_t LOWER_RIGHT = segment(3);
    constexpr uint16_t BOTTOM = segment(4);
    constexpr uint16_t UPPER_LEFT_DIAGONAL = segment(8);
    constexpr uint16_t UPPER_RIGHT_DIAGONAL = segment(10);
    constexpr uint16_t LOWER_LEFT_DIAGONAL = segment(14);
    constexpr uint16_t LOWER_RIGHT_DIAGONAL = segment(12);

    constexpr uint16_t W = UPPER_LEFT | UPPER_RIGHT | LOWER_LEFT |
                           LOWER_RIGHT | LOWER_LEFT_DIAGONAL |
                           LOWER_RIGHT_DIAGONAL;
    constexpr uint16_t E = TOP | UPPER_LEFT | MIDDLE_LEFT |
                           MIDDLE_RIGHT | LOWER_LEFT | BOTTOM;
    constexpr uint16_t L = UPPER_LEFT | LOWER_LEFT | BOTTOM;
    constexpr uint16_t C = TOP | UPPER_LEFT | LOWER_LEFT | BOTTOM;
    constexpr uint16_t O = TOP | UPPER_LEFT | UPPER_RIGHT |
                           LOWER_LEFT | LOWER_RIGHT | BOTTOM;
    constexpr uint16_t M = UPPER_LEFT | UPPER_RIGHT | LOWER_LEFT |
                           LOWER_RIGHT | UPPER_LEFT_DIAGONAL |
                           UPPER_RIGHT_DIAGONAL;

    // GR8 is leftmost and GR1 is rightmost. Leave the rightmost position blank.
    const uint16_t grids[8] = {0, E, M, O, C, L, E, W};
    showGridPatterns(grids);
}

void PT6312::showCd()
{
    constexpr uint16_t TOP = segment(1);
    constexpr uint16_t UPPER_LEFT = segment(6);
    constexpr uint16_t UPPER_RIGHT = segment(2);
    constexpr uint16_t MIDDLE_LEFT = segment(7);
    constexpr uint16_t MIDDLE_RIGHT = segment(11);
    constexpr uint16_t LOWER_LEFT = segment(5);
    constexpr uint16_t LOWER_RIGHT = segment(3);
    constexpr uint16_t BOTTOM = segment(4);
    constexpr uint16_t C = TOP | UPPER_LEFT | LOWER_LEFT | BOTTOM;
    // Uppercase D uses the center vertical strokes S9 and S13 so it remains
    // visibly different from O on the Sony 14-segment character.
    constexpr uint16_t D = TOP | UPPER_RIGHT | LOWER_RIGHT | BOTTOM |
                           segment(9) | segment(13);

    // Center CD in the eight available character positions.
    const uint16_t grids[8] = {0, 0, 0, D, C, 0, 0, 0};
    showGridPatterns(grids);
}

void PT6312::showTextWindow(const char *text, int16_t offset)
{
    uint16_t grids[8] = {};
    const int16_t length = static_cast<int16_t>(strlen(text));

    for (int16_t position = 0; position < 8; ++position) {
        const int16_t textIndex = offset + position;
        if (textIndex >= 0 && textIndex < length) {
            // Screen position zero is the leftmost VFD character (GR8).
            grids[7 - position] = glyphFor(text[textIndex]);
        }
    }

    showGridPatterns(grids);
}

void PT6312::showGridPatterns(const uint16_t patterns[8])
{
    sendCommand(COMMAND_WRITE_AUTO_INCREMENT);
    digitalWrite(strobePin_, LOW);
    writeByteLsbFirst(COMMAND_ADDRESS_ZERO);
    for (uint8_t grid = 0; grid < 8; ++grid) {
        writeByteLsbFirst(static_cast<uint8_t>(patterns[grid] & 0xFF));
        writeByteLsbFirst(static_cast<uint8_t>((patterns[grid] >> 8) & 0x3F));
    }
    for (uint8_t index = 16; index < DISPLAY_RAM_BYTES; ++index)
        writeByteLsbFirst(0x00);
    digitalWrite(strobePin_, HIGH);
    setDisplayEnabled(true, 7);
}

void PT6312::sendCommand(uint8_t command)
{
    digitalWrite(strobePin_, LOW);
    writeByteLsbFirst(command);
    digitalWrite(strobePin_, HIGH);
}

void PT6312::writeByteLsbFirst(uint8_t value)
{
    for (uint8_t bit = 0; bit < 8; ++bit) {
        digitalWrite(clockPin_, LOW);
        digitalWrite(dinPin_, (value & 0x01U) ? HIGH : LOW);
        delayMicroseconds(2);
        digitalWrite(clockPin_, HIGH);
        delayMicroseconds(2);
        value >>= 1;
    }
    digitalWrite(clockPin_, LOW);
}
