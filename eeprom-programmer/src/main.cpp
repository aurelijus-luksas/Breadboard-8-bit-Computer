#include "pinout.h"

#include <progressBar.h>

void setIdle() {
    // disable writes
    pinMode(Pinout::writeEnablePin, OUTPUT);
    digitalWrite(Pinout::writeEnablePin, HIGH);

    for (const auto pin : Pinout::addressPins) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
    }

    for (const auto pin : Pinout::dataPins) {
        pinMode(pin, INPUT);
    }
}

void setAddress(uint32_t address) {
    for (size_t i = 0; i < Pinout::addressPins.size(); i++) {
        int bit = (address >> i) & 1;
        digitalWrite(Pinout::addressPins[i], bit);
    }
}

uint8_t readByte(uint32_t address) {
    for (const auto pin : Pinout::dataPins) {
        pinMode(pin, INPUT);
    }
    digitalWrite(LED_BUILTIN, HIGH);
    setAddress(address);

    uint8_t data = 0;
    for (size_t i = 0; i < Pinout::dataPins.size(); i++) {
        if (digitalRead(Pinout::dataPins[i])) {
            data |= (1 << i);
        }
    }
    return data;
}

void writeByte(uint32_t address, uint8_t data) {
    pinMode(Pinout::writeEnablePin, OUTPUT);
    digitalWrite(Pinout::writeEnablePin, HIGH);

    setAddress(address);

    for (const auto pin : Pinout::dataPins) {
        pinMode(pin, OUTPUT);
    }

    for (size_t i = 0; i < Pinout::dataPins.size(); i++) {
        digitalWrite(Pinout::dataPins[i], (data >> i) & 1);
    }

    digitalWrite(Pinout::writeEnablePin, LOW);
    delayMicroseconds(1);
    digitalWrite(Pinout::writeEnablePin, HIGH);
    delay(10); // data is written within 10ms
}

void dumpEEPROM() {
    Serial.println("Dumping EEPROM contents:");

    uint32_t addressCount = 2048;
    int rowSize = 16;

    uint8_t dataBuffer[rowSize];
    char stringBuffer[rowSize * 3 + 16];
    for (uint32_t address = 0; address < addressCount; address += rowSize) {
        for (int offset = 0; offset < rowSize; offset++) {
            dataBuffer[offset] = readByte(address + offset);
            ProgressBar::setProgress(static_cast<float>(address + offset) / addressCount);
        }

        int stringIndex = 0;
        stringIndex += sprintf(stringBuffer + stringIndex, "0x%03X:", address);
        for (int offset = 0; offset < rowSize; offset++) {
            if (offset % 8 == 0) {
                stringIndex += sprintf(stringBuffer + stringIndex, " ");
            }
            stringIndex += sprintf(stringBuffer + stringIndex, "%02X ", dataBuffer[offset]);
        }
        Serial.println(stringBuffer);
    }

    ProgressBar::setProgress(0.0);
    setIdle();
}

void writeNumberDisplay();
void writeMicrocode();

void writeEEPROM(void (*patternFunc)()) {
    Serial.println("Writing to EEPROM...");

    patternFunc();

    Serial.println("Write complete.");
    ProgressBar::setProgress(0.0);
    setIdle();
}

void eraseEEPROM() {
    Serial.println("Erasing EEPROM...");

    for (const auto pin : Pinout::dataPins) {
        pinMode(pin, OUTPUT);
    }

    for (uint32_t address = 0; address < 2048; address++) {
        writeByte(address, 0xFF);
        ProgressBar::setProgress(static_cast<float>(address) / 2048.0f);
    }
    Serial.println("Erase complete.");
    ProgressBar::setProgress(0.0);
    setIdle();
}

void setup() {
    setIdle();

    Serial.begin(9600);
    ProgressBar::init();

    ProgressBar::setProgress(0.0);

    Serial.println("EEPROM Programmer Ready. Send 'h' for help.");
}

void loop() {
    if (Serial.available() > 0) {
        char input = Serial.read();
        Serial.print("received: ");
        Serial.print(input);
        Serial.print(" (");
        Serial.print((int)input);
        Serial.println(")");

        if (input == 'h' || input == 'H') {
            Serial.println("Commands:");
            Serial.println("  r: Read EEPROM contents (make sure OE is LOW)");
            Serial.println("  e: Erase EEPROM (make sure OE is HIGH)");
            Serial.println("  n: Write number display pattern to EEPROM (make sure OE is HIGH)");
            Serial.println("  m: Write microcode to EEPROM (make sure OE is HIGH)");
        }

        if (input == 'r') {
            Serial.println("Make sure yellow wire (OE on EEPROM) is set to LOW before reading. Use 'R' to proceed with reading.");
        } else if (input == 'R') {
            dumpEEPROM();
        }

        if (input == 'n') {
            Serial.println("Make sure yellow wire (OE on EEPROM) is set to HIGH before writing number display pattern. Use 'N' to proceed "
                           "with writing.");
        } else if (input == 'N') {
            writeEEPROM(writeNumberDisplay);
        }

        if (input == 'm') {
            Serial.println(
                "Make sure yellow wire (OE on EEPROM) is set to HIGH before writing microcode. Use 'M' to proceed with writing.");
        } else if (input == 'M') {
            writeEEPROM(writeMicrocode);
        }

        if (input == 'e') {
            Serial.println("Make sure yellow wire (OE on EEPROM) is set to HIGH before erasing. Use 'E' to proceed with erasing.");
        } else if (input == 'E') {
            eraseEEPROM();
        }
    }
}
// ######################################################################################################## Number display
void writeNumberDisplay() {
    Serial.println("Writing number display pattern to EEPROM...");

    int progress = 0;
    float maxProgress = 1024.0f;
    constexpr uint16_t signedMask = 0b10000000000;

    const uint8_t digits[] = {0x7e, 0x30, 0x6d, 0x79, 0x33, 0x5b, 0x5f, 0x70, 0x7f, 0x7b};

    // ones place
    for (uint32_t value = 0; value < 256; value++) {
        const uint16_t unsignedAddress = 0b00000000000 | value;
        const uint16_t signedAddress = signedMask | unsignedAddress;

        writeByte(unsignedAddress, digits[uint8_t(value) % 10]);
        writeByte(signedAddress, digits[abs(int8_t(value)) % 10]);

        ProgressBar::setProgress(++progress / maxProgress);
    }

    // tens place
    for (uint32_t value = 0; value < 256; value++) {
        const uint16_t unsignedAddress = 0b00100000000 | value;
        const uint16_t signedAddress = signedMask | unsignedAddress;

        writeByte(unsignedAddress, digits[uint8_t(value) / 10 % 10]);
        writeByte(signedAddress, digits[abs(int8_t(value) / 10) % 10]);

        ProgressBar::setProgress(++progress / maxProgress);
    }

    // hundreds place
    for (uint32_t value = 0; value < 256; value++) {
        const uint16_t unsignedAddress = 0b01000000000 | value;
        const uint16_t signedAddress = signedMask | unsignedAddress;

        writeByte(unsignedAddress, digits[uint8_t(value) / 100 % 10]);
        writeByte(signedAddress, digits[abs(int8_t(value) / 100) % 10]);

        ProgressBar::setProgress(++progress / maxProgress);
    }

    // sign bit
    for (uint32_t value = 0; value < 256; value++) {
        const uint16_t unsignedAddress = 0b01100000000 | value;
        const uint16_t signedAddress = signedMask | unsignedAddress;

        writeByte(unsignedAddress, 0x00);
        writeByte(signedAddress, int8_t(value) < 0 ? 0x01 : 0x00);

        ProgressBar::setProgress(++progress / maxProgress);
    }
}

// ######################################################################################################## Microcode
#define HLT 0b1000000000000000 // Halt clock
#define MI  0b0100000000000000 // Memory address register in
#define RI  0b0010000000000000 // RAM data in
#define RO  0b0001000000000000 // RAM data out
#define IO  0b0000100000000000 // Instruction register out
#define II  0b0000010000000000 // Instruction register in
#define AI  0b0000001000000000 // A register in
#define AO  0b0000000100000000 // A register out
#define EO  0b0000000010000000 // ALU out
#define SU  0b0000000001000000 // ALU subtract
#define BI  0b0000000000100000 // B register in
#define OI  0b0000000000010000 // Output register in
#define CE  0b0000000000001000 // Program counter enable
#define CO  0b0000000000000100 // Program counter out
#define J   0b0000000000000010 // Jump (program counter in)
#define FI  0b0000000000000001 // Flags in

#define FLAGS_Z0C0 0
#define FLAGS_Z0C1 1
#define FLAGS_Z1C0 2
#define FLAGS_Z1C1 3

#define JC 0b0111
#define JZ 0b1000

// clang-format off
uint16_t UCODE_TEMPLATE[16][8] = {
  { MI|CO,  RO|II|CE,  0,      0,      0,           0, 0, 0 },   // 0000 - NOP
  { MI|CO,  RO|II|CE,  IO|MI,  RO|AI,  0,           0, 0, 0 },   // 0001 - LDA
  { MI|CO,  RO|II|CE,  IO|MI,  RO|BI,  EO|AI|FI,    0, 0, 0 },   // 0010 - ADD
  { MI|CO,  RO|II|CE,  IO|MI,  RO|BI,  EO|AI|SU|FI, 0, 0, 0 },   // 0011 - SUB
  { MI|CO,  RO|II|CE,  IO|MI,  AO|RI,  0,           0, 0, 0 },   // 0100 - STA
  { MI|CO,  RO|II|CE,  IO|AI,  0,      0,           0, 0, 0 },   // 0101 - LDI
  { MI|CO,  RO|II|CE,  IO|J,   0,      0,           0, 0, 0 },   // 0110 - JMP
  { MI|CO,  RO|II|CE,  0,      0,      0,           0, 0, 0 },   // 0111 - JC
  { MI|CO,  RO|II|CE,  0,      0,      0,           0, 0, 0 },   // 1000 - JZ
  { MI|CO,  RO|II|CE,  0,      0,      0,           0, 0, 0 },   // 1001
  { MI|CO,  RO|II|CE,  0,      0,      0,           0, 0, 0 },   // 1010
  { MI|CO,  RO|II|CE,  0,      0,      0,           0, 0, 0 },   // 1011
  { MI|CO,  RO|II|CE,  0,      0,      0,           0, 0, 0 },   // 1100
  { MI|CO,  RO|II|CE,  0,      0,      0,           0, 0, 0 },   // 1101
  { MI|CO,  RO|II|CE,  AO|OI,  0,      0,           0, 0, 0 },   // 1110 - OUT
  { MI|CO,  RO|II|CE,  HLT,    0,      0,           0, 0, 0 },   // 1111 - HLT
};
// clang-format on

void writeMicrocode() {
    Serial.println("Writing microcode to EEPROM...");

    uint16_t ucode[4][16][8];

    // ZF = 0, CF = 0
    memcpy(ucode[FLAGS_Z0C0], UCODE_TEMPLATE, sizeof(UCODE_TEMPLATE));

    // ZF = 0, CF = 1
    memcpy(ucode[FLAGS_Z0C1], UCODE_TEMPLATE, sizeof(UCODE_TEMPLATE));
    ucode[FLAGS_Z0C1][JC][2] = IO | J;

    // ZF = 1, CF = 0
    memcpy(ucode[FLAGS_Z1C0], UCODE_TEMPLATE, sizeof(UCODE_TEMPLATE));
    ucode[FLAGS_Z1C0][JZ][2] = IO | J;

    // ZF = 1, CF = 1
    memcpy(ucode[FLAGS_Z1C1], UCODE_TEMPLATE, sizeof(UCODE_TEMPLATE));
    ucode[FLAGS_Z1C1][JC][2] = IO | J;
    ucode[FLAGS_Z1C1][JZ][2] = IO | J;

    int progress = 0;
    float maxProgress = 1024.f;

    for (int address = 0; address < 1024; address += 1) {
        int flags = (address & 0b1100000000) >> 8;
        int byte_sel = (address & 0b0010000000) >> 7;
        int instruction = (address & 0b0001111000) >> 3;
        int step = (address & 0b0000000111);

        if (byte_sel) {
            writeByte(address, ucode[flags][instruction][step]);
        } else {
            writeByte(address, ucode[flags][instruction][step] >> 8);
        }

        ProgressBar::setProgress(++progress / maxProgress);
    }
}