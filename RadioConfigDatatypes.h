#pragma once
#include <Arduino.h>

namespace Commands{
    constexpr uint8_t WRITE_PERMANENT = 0xC0;
    constexpr uint8_t READ = 0xC1;
    constexpr uint8_t WRITE_TEMPORARY = 0xC2;
    constexpr uint8_t REMOTE_PREAMBLE[2] = {0xCF, 0xCF};
    constexpr uint8_t READ_AMBIENT_RSSI[6] = {0xC0, 0xC1, 0xC2, 0xC3, 0x00, 0x01};
    constexpr uint8_t READ_BOTH_RSSI[6] = {0xC0, 0xC1, 0xC2, 0xC3, 0x00, 0x02};
    constexpr uint8_t READ_RSSI[6] = {0xC0, 0xC1, 0xC2, 0xC3, 0x01, 0x01};
};

namespace RadioConfigTypes
{

// datatypes for radio configuration stuff
enum RadioMode {
    Normal = 0b00,
    WakeUp = 0b01,
    Program = 0b10,
    PowerDown = 0b11
};

enum ParityConfig {
    Parity_8N1 = 0b00,
    Parity_8O1 = 0b01,
    Parity_8E1 = 0b10
};

enum SerialSpeeds { // in baud (bits/s)
    BAUD_1200 = 0b000,
    BAUD_2400 = 0b001,
    BAUD_4800 = 0b010,
    BAUD_9600 = 0b011,
    BAUD_19200 = 0b100,
    BAUD_38400 = 0b101,
    BAUD_57600 = 0b110,
    BAUD_115200 = 0b111
};

enum AirDataRate { // in bits/s
    RATE_2400 = 0b011, // all between 0b000 and 0b001 are the same?
    RATE_4800 = 0b100,
    RATE_9600 = 0b101,
    RATE_15600 = 0b110 // and 0b111?
};

enum PacketSize { // in bytes
    SIZE_240 = 0b00,
    SIZE_128 = 0b01,
    SIZE_64 = 0b10,
    SIZE_32 = 0b11
};

enum EnableRSSIReadings {
    Disabled = 0b0,
    Enabled = 0b1
};

enum TransmitPower {
    dBm33 = 0b00,
    dBm30 = 0b01,
    dBm27 = 0b10,
    dBm24 = 0b11
};

enum Destination {
    Broadcast = 0b0,
    Unicast = 0b1
};

enum RelayMode {
    RelayDisabled = 0b0,
    RelayEnabled = 0b1
};

enum EnableListenBeforeTX { // prevents TXing on another source
    LBTDisabled = 0b0,
    LBTEnabled = 0b1
};

enum WakeOnReceiveMode {
    NormalWOR = 0b0,
    ListenOnly = 0b1
};

enum WakeOnReceiveListenPeriod { // in milliseconds
    TIME_500 = 0b000,
    TIME_1000 = 0b001,
    TIME_1500 = 0b010,
    TIME_2000 = 0b011,
    TIME_2500 = 0b100,
    TIME_3000 = 0b101,
    TIME_3500 = 0b110,
    TIME_4000 = 0b111
};


enum ConfigRegisters {
    AddressHigh = 0x00,
    AddressLow = 0x01,
    
    NetworkID = 0x02,
    
    SerialConfigRegister = 0x03, // Configures UART speed, UART parity, and Air data rate
    RadioConfigRegister = 0x04, // Configures Packet Size, RSSI readings, and TX power
    FrequencyChannel = 0x05, // check datasheet for how to map
    OptionConfigRegister = 0x06,
    
    EncryptionHighByte = 0x07,
    EncryptionLowByte = 0x08,

    ProductIDStartByte = 0x80,
    ProductIDEndByte = 0x86,
};

enum ConfigRegisterLengths {
    ConfigRegisterLength = 0x09,
    ProductIDLength = 0x07
};

};

struct RadioConfig
{
    uint32_t frequency; // kHz
    uint16_t address;
    uint8_t networkId;
    uint16_t encryptionKey;
    RadioConfigTypes::ParityConfig parityConfig;
    RadioConfigTypes::SerialSpeeds serialSpeed;
    RadioConfigTypes::AirDataRate airDataRate;
    RadioConfigTypes::PacketSize packetSize;
    RadioConfigTypes::EnableRSSIReadings ambientRSSIEnabled;
    RadioConfigTypes::EnableRSSIReadings rssiReadingsEnabled;
    RadioConfigTypes::TransmitPower txPower;
    RadioConfigTypes::Destination destination;
    RadioConfigTypes::RelayMode relayMode;
    RadioConfigTypes::EnableListenBeforeTX listenBeforeTxEnable;
    RadioConfigTypes::WakeOnReceiveMode worMode;
    RadioConfigTypes::WakeOnReceiveListenPeriod worPeriod;

    bool operator==(const RadioConfig& other) const
    {
        return frequency == other.frequency &&
               address == other.address &&
               networkId == other.networkId &&
               encryptionKey == other.encryptionKey &&
               parityConfig == other.parityConfig &&
               serialSpeed == other.serialSpeed &&
               airDataRate == other.airDataRate &&
               packetSize == other.packetSize &&
               ambientRSSIEnabled == other.ambientRSSIEnabled &&
               rssiReadingsEnabled == other.rssiReadingsEnabled &&
               txPower == other.txPower &&
               destination == other.destination &&
               relayMode == other.relayMode &&
               listenBeforeTxEnable == other.listenBeforeTxEnable &&
               worMode == other.worMode &&
               worPeriod == other.worPeriod;
    }

    bool operator!=(const RadioConfig& other) const
    {
        return !(*this == other);
    }

    void print(){
        Serial.println("freq:" + String(frequency));
        Serial.println("addr:" + String(address));
        Serial.println("netid:" + String(networkId));
        Serial.println("key:" + String(encryptionKey));
        Serial.println("parity:" + String(parityConfig));
        Serial.println("serial:" + String(serialSpeed));
        Serial.println("air:" + String(airDataRate));
        Serial.println("packet:" + String(packetSize));
        Serial.println("ambient:" + String(ambientRSSIEnabled));
        Serial.println("rssi:" + String(rssiReadingsEnabled));
        Serial.println("power:" + String(txPower));
        Serial.println("dest:" + String(destination));
        Serial.println("relay:" + String(relayMode));
        Serial.println("wor:" + String(worMode));
        Serial.println("period:" + String(worPeriod));
    }
};

struct ConfigStatus{
    RadioConfig config;
    bool readSuccessfully;
};