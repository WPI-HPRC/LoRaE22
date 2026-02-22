#pragma once
#include <Arduino.h>
#include <HardwareSerial.h>
#include "RadioConfigDatatypes.h"
// #include "RxMessageBuffer.h"
#include "TxByteBuffer.h"
#include "RxBuffer.h"

#define BYTE_BUFFER_SIZE 1024
#define MAX_MESSAGE_SIZE (240 - 6 - 2) // we want a message to fit into one packet
#define MESSAGE_BUFFER_SIZE 128

using namespace RadioConfigTypes;

class LoRaE22 {

    public:
        LoRaE22(HardwareSerial *serialPort, uint8_t Pin_M0, uint8_t Pin_M1, uint8_t Pin_AUX, const char* Callsign)
            : callsign(Callsign), rxBuffer(BYTE_BUFFER_SIZE, MESSAGE_BUFFER_SIZE, Callsign), serial(serialPort), M0(Pin_M0), M1(Pin_M1), AUX(Pin_AUX) {};

        int8_t init(unsigned char allowedAttempts);
        void changeSerialPortCallback(bool (*fptr)(RadioConfigTypes::SerialSpeeds, RadioConfigTypes::ParityConfig)){changeSerialConfiguration = fptr; };

        void update();

        // do we have a message
        bool hasMessage();
        // get a message
        bool getMessage(uint8_t *buffer, size_t bufferLength, uint16_t& messageLength);
        // send a message
        bool sendMessage(const uint8_t* data, size_t length);

        // THIS WILL IMMEDIEATELY CHANGE MODE. Please wait for the module to be ready using `ModuleReady()` or `waitForModule()`
        void setMode(RadioConfigTypes::RadioMode mode);

        /// @brief This sets the value only in uC memory. Write to the radio with writeConfigPersistent() or writeConfigTemporary()
        /// @param config Config we want to set. Please ensure that the `frequency` field is within legal range for your band
        void setConfig(RadioConfig config){ radioConfig = config; };

        /// @brief Public facing function that includes legality checking for setting frequency.
        /// @param freqMHz Desired frequency in MHz. Please set in 125kHz steps.
        /// @return true if was able to set successfully (matches laws). Returns false otherwise
        bool setFrequency(float freqMHz);

        // get methods
        RadioConfig getConfig(){return radioConfig; };

        size_t dataAvailable(){ return serial->available(); };

        // DO NOT USE
        uint8_t getByte();
        // DO NOT USE
        bool getDataStruct(const void *dataStruct, size_t _size);

        // DO NOT USE
        void sendByte(uint8_t _byte);
        // DO NOT USE
        bool sendDataStruct(const void *dataStruct, size_t _size);

        // Reads AUX
        // very confusing to use, better documentation will be done later
        bool moduleReady();
        void waitForModule(){while(!moduleReady()){yield();};};
        
        // write to radio registers
        size_t buildConfigBuffer(uint8_t* buffer);
        bool writeConfigPersistent(uint8_t* configBuffer, uint8_t length);
        bool writeConfigTemporary(uint8_t* configBuffer, uint8_t length);
        bool remoteWriteConfigPersistent(uint8_t* configBuffer, uint8_t length);
        bool remoteWriteConfigTemporary(uint8_t* configBuffer, uint8_t length);

        int8_t getRSSIAmbientNoise();
        int8_t getBothRSSI();
        int8_t getRSSILastRX();

        // read from radio registers
        int8_t checkConfigMatches();
        bool readConfigIntoMemory();
        bool readProductInfo();

    private:
        // read the actual register
        ConfigStatus readConfigRegisters();

        const char* callsign;

        // have an input and output buffer
        uint16_t maxMessageSize = MAX_MESSAGE_SIZE;
        uint8_t maxMessageCount = MESSAGE_BUFFER_SIZE;
        RxBuffer rxBuffer;
        TxByteBuffer<BYTE_BUFFER_SIZE> txBuffer;

        // form bytes
        uint8_t formSerialConfigByte();
        uint8_t formRadioConfigByte();
        uint8_t formFrequencyByte();
        uint8_t formOptionConfigByte();

        // de-form bytes
        void deformSerialConfigByte(uint8_t byteIn, RadioConfig *config);
        void deformRadioConfigByte(uint8_t byteIn, RadioConfig *config);
        float deformFrequencyByte(uint8_t byteIn);
        void deformOptionConfigByte(uint8_t byteIn, RadioConfig *config);

        // hardware interfaces
        HardwareSerial *serial;
        bool (*changeSerialConfiguration)(RadioConfigTypes::SerialSpeeds, RadioConfigTypes::ParityConfig);
        uint8_t M0, M1, AUX;

        // config values
        RadioConfigTypes::RadioMode radioMode;
        // register values
        RadioConfig radioConfig;
        uint8_t configBuffer[10];
        uint8_t productInfo[7];


        ParityConfig getParityConfig(int);
        SerialSpeeds getSerialSpeed(int);
        AirDataRate getAirDataRate(int);
        PacketSize getPacketSize(int);
        EnableRSSIReadings getEnableRSSI(int);
        TransmitPower getTransmitPower(int);
        Destination getDestination(int);
        RelayMode getRelayMode(int);
        EnableListenBeforeTX getListenBeforeTX(int);
        WakeOnReceiveMode getWORMode(int);
        WakeOnReceiveListenPeriod getWORPeriod(int);
};