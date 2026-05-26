#pragma once

#include <Arduino.h>
#include <array>

namespace Pinout {

// 1 Write Enable Pin
constexpr pin_size_t writeEnablePin = D13; // EEPROM WE

// 11 Address pins ordered from A0 to A10
constexpr std::array<pin_size_t, 11> addressPins = {
    A2,  // EEPROM A0
    A1,  // EEPROM A1
    A0,  // EEPROM A2
    D5,  // EEPROM A3
    D6,  // EEPROM A4
    D7,  // EEPROM A5
    D8,  // EEPROM A6
    D9,  // EEPROM A7
    D12, // EEPROM A8
    D11, // EEPROM A9
    D10, // EEPROM A10
};

// 8 Data pins ordered from I/O_0 to I/O_7
constexpr std::array<pin_size_t, 8> dataPins = {
    A3, // EEPROM I/O_0
    A4, // EEPROM I/O_1
    A5, // EEPROM I/O_2
    D0, // EEPROM I/O_3
    D1, // EEPROM I/O_4
    D2, // EEPROM I/O_5
    D3, // EEPROM I/O_6
    D4, // EEPROM I/O_7
};

} // namespace Pinout
