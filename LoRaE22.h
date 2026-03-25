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
#define BAND_LOW_MHZ 222.000
#define BAND_HIGH_MHZ 225.000
#define CHANNEL_WIDTH_KHZ 250
#define BASE_FREQ_KHZ 220125

using namespace RadioConfigTypes;

class LoRaE22 {

    public:
        LoRaE22(HardwareSerial *serialPort, uint8_t Pin_M0, uint8_t Pin_M1, uint8_t Pin_AUX, const char* Callsign)
            : callsign(Callsign), rxBuffer(BYTE_BUFFER_SIZE, MESSAGE_BUFFER_SIZE, Callsign), serial(serialPort), M0(Pin_M0), M1(Pin_M1), AUX(Pin_AUX) {};

        int8_t init(unsigned char allowedAttempts);
        void changeSerialPortCallback(bool (*fptr)(RadioConfigTypes::SerialSpeeds, RadioConfigTypes::ParityConfig)){changeSerialConfiguration = fptr; };
        // we occasionally want to wait for the module to complete something. 
        // this sets how long we wait before giving up and returning early
        // should be greater than 1000ms (experimental testing)
        void setTimeout(unsigned long TimeoutMS){timeoutMS = TimeoutMS;};

        void update();

        // do we have a message
        bool hasMessage();
        // get a messagef
        bool getMessage(uint8_t *buffer, size_t bufferLength, uint16_t& messageLength);
        // send a message
        bool sendMessage(const uint8_t* data, size_t length);

        // if the module is fine to be changed, this will change mode immediately
        bool setMode(RadioConfigTypes::RadioMode mode);

        /// @brief This sets the value only in uC memory. Write to the radio with writeConfigPersistent() or writeConfigTemporary()
        /// @param config Config we want to set. Please ensure that the `frequency` field is within legal range for your band
        void setConfig(RadioConfig config){ radioConfig = config; };

        /// @brief Public facing function that includes legality checking for setting frequency.
        /// @param freqMHz Desired frequency in MHz. Please set in 250KHz steps.
        /// @return true if was able to set successfully (matches laws). Returns false otherwise
        bool setFrequency(float freqMHz);

        // get methods
        RadioConfig getConfig(){return radioConfig; };

        size_t dataAvailableCount(){ return serial->available(); };

        // Primarily for testing purposes
        void sendByte(uint8_t _byte);
        uint8_t getByte();

        // Reads AUX
        // this is very pooly documentated in the datasheet, and the datasheet seems to be somewhat wrong.
        // to my best understanding, the module uses this as a "ready" indicator.
        // when the module is "not ready", it may:
        // - drop bytes that are sent to it to be transmitted, due to full internal buffer
        // - drop received bytes in its internal buffer
        // - not recognize operating mode change
        // - not recognize new configuration commands
        // pay careful attention to this
        bool moduleReady();
        // waits for moduleReady() to become true (with timeout)
        // this is very pooly documentated in the datasheet, and the datasheet seems to be somewhat wrong.
        // to my best understanding, the module uses this as a "ready" indicator.
        // when the module is "not ready", it may:
        // - drop bytes that are sent to it to be transmitted, due to full internal buffer
        // - drop received bytes in its internal buffer
        // - not recognize operating mode change
        // - not recognize new configuration commands
        // pay careful attention to this
        void waitForModule();
        
        // write to radio registers
        size_t buildConfigBuffer(uint8_t* buffer);
        bool writeConfigPersistent(uint8_t* configBuffer, size_t length);
        bool writeConfigTemporary(uint8_t* configBuffer, size_t length);
        bool remoteWriteConfigPersistent(uint8_t* configBuffer, size_t length);
        bool remoteWriteConfigTemporary(uint8_t* configBuffer, size_t length);

        void requestRSSIAmbientNoise();
        void requestBothRSSI();
        void requestRSSILastRX();

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
        uint32_t deformFrequencyByte(uint8_t byteIn);
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

        unsigned long timeoutMS = 0;
        unsigned long long timeoutStart =0;
        bool firstCall = true;


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