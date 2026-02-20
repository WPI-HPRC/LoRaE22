#include "./LoRaE22.h"

// #define DEBUG

/// @brief BLOCKING. Please feed in config & changeSerialConfig callback prior to calling.
/// handles all the crap to init the module. DO NOT set pinModes or anything prior to calling, this function does it all.
/// @param allowedAttempts number of times we let it try to program/read the radio module before giving up
/// @return 0 if init OK and module was already programmed, 1 if module initied but had to be programmed, -1 if module failed to init
int8_t LoRaE22::init(uint8_t allowedAttempts)
{
    // configure our additional hardware pins
    // pinMode(AUX, INPUT);
    pinMode(M0, OUTPUT);
    pinMode(M1, OUTPUT);

    // inital config for programming
    changeSerialConfiguration(RadioConfigTypes::SerialSpeeds::BAUD_9600, RadioConfigTypes::ParityConfig::Parity_8N1);

    setMode(RadioMode::Normal);
    
    delay(3);

    // tight loop if the radio module isn't ready
    waitForModule();

    #ifdef DEBUG
        SerialUSB.println("module ready");
    #endif

    
    #ifdef DEBUG
        SerialUSB.println("entering programming mode");
    #endif

    // read out the existing config
    setMode(RadioMode::Program);

    delay(3);

    // tight loop meh badness
    waitForModule();
    
    #ifdef DEBUG
        SerialUSB.println("reading config");
    #endif

    

    size_t attemptCounter = 0;
    int8_t status;
    do{
        status = checkConfigMatches();
        attemptCounter++;
    }
    while(allowedAttempts != 0 && status == -1 && attemptCounter < allowedAttempts);

    if(status == 0){ 
        setMode(RadioMode::Normal);
        waitForModule();
        return 0; 
    }; // read successfully and matches, we're good! early returns goated
    if(status == -1){ return -1; }; // uh-oh. we failed to read (means module failed to init properly). early return
    // keep going if status == 1 (config read successfully but doesn't match)

    // if our config didn't match, then we write out config to the radio module to make it match!
    #ifdef DEBUG
        SerialUSB.println("config didn't match, writing config");
    #endif

    // let's write our config
    size_t length = buildConfigBuffer((uint8_t*)configBuffer); // build config buffer

    attemptCounter = 0;
    bool success = true;
    do {
        waitForModule(); // wait for module to be ready
        success = writeConfigPersistent(configBuffer, length); // write the dang config
        attemptCounter++;
    }
    while(allowedAttempts != 0 && !success && attemptCounter < allowedAttempts);

    if(!success){return -1;};


    #ifdef DEBUG
        SerialUSB.println("config written");
        SerialUSB.println(success);
    #endif

    // wait for module to complete

    waitForModule();
    changeSerialConfiguration(radioConfig.serialSpeed, radioConfig.parityConfig);

    // set radio back to normal operating mode
    setMode(RadioMode::Normal);

    // wait for the module to complete transistion
    waitForModule();
    #ifdef DEBUG
        SerialUSB.println("module in normal mode, reconfiguring serial port");
    #endif
    
    // reconfigure our serial port to the correct value now
    

    // we're done!! (and only had 5 tight loops in this function...)
    return 1; // init sucessfully, had to program radio
};

void LoRaE22::update()
{
    // push any serial data we have to the receieve buffer
    while(dataAvailable()){
        uint8_t byte = serial->read();
        // SerialUSB.printf("%0X     ",byte);
        rxBuffer.pushByte(byte);
    }


    // // push any messages in the buffer out to the module
    // if(!txBuffer.empty()){
    //     uint8_t byte;
    //     txBuffer.pop(byte);

    //     serial->write(byte);
    // }

    return;
}

bool LoRaE22::hasMessage()
{
    return !rxBuffer.isMsgEmpty();
}

bool LoRaE22::getMessage(uint8_t* buffer, size_t bufferLength, uint16_t& messageLength)
{
    if(rxBuffer.isMsgEmpty()){
        return false;
    }

    // uint8_t dataBuf[240] = new uint8_t[240];
    Message msgRx = rxBuffer.getFrontMessage();
    size_t len = msgRx.length;

    if(len > bufferLength){
        memcpy(buffer, msgRx.data, bufferLength);
        return false;
    }

    memcpy(buffer, msgRx.data, len);
    messageLength = len;
    
    rxBuffer.popMessage();

    return true;
}

bool LoRaE22::sendMessage(const uint8_t* data, size_t length)
{

    // write callsign
    for(size_t i = 0; i<strlen(callsign); i++){
        serial->write(callsign[i]);
    }
    // insert payload length
    serial->write(length & 0xFF);


    size_t lengthWritten = serial->write(data, length);

    return (length == lengthWritten);
}

// bool LoRaE22::sendMessage(const uint8_t* data, size_t length)
// {
//     // can't send bad messages
//     if(length == 0 || length > maxMessageSize){
//         return false;
//     }

//     // insert callsign
//     for(size_t i = 0; i<strlen(callsign); i++){
//         txBuffer.push(callsign[i]);
//     }

//     // insert payload length (little endian order)
//     txBuffer.push(length && 0xFF);
//     txBuffer.push((length >> 8) & 0xFF);

//     // insert actual payload
//     for(uint16_t i = 0; i<length; i++){
//         txBuffer.push(data[i]);
//     }

//     return true;
// }


bool LoRaE22::moduleReady()
{
    return digitalRead(AUX);
}

void LoRaE22::setMode(RadioConfigTypes::RadioMode mode)
{
    
    radioMode = mode;
    switch(mode){
        case RadioConfigTypes::RadioMode::Normal:
            digitalWrite(M0, 0);
            digitalWrite(M1, 0);
            break;
        case RadioConfigTypes::RadioMode::WakeUp:
            digitalWrite(M0, 1);
            digitalWrite(M1, 0);
            break;
        case RadioConfigTypes::RadioMode::Program:
            digitalWrite(M0, 0);
            digitalWrite(M1, 1);
            break;
        case RadioConfigTypes::RadioMode::PowerDown:
            digitalWrite(M0, 1);
            digitalWrite(M1, 1);
            break;
    }
};

bool LoRaE22::setFrequency(float freqMHz){
    if(freqMHz < 222.000 || freqMHz > 225.000){return false;}; // ILLEGAL per FCC.
    radioConfig.frequency = (int)(freqMHz*1000.0);
    return true;
}

uint8_t LoRaE22::getByte()
{
    return serial->read();
};

void LoRaE22::sendByte(uint8_t _byte)
{
    serial->write(_byte); //serial->flush();
};

bool LoRaE22::sendDataStruct(const void* dataStruct, size_t _size)
{
    size_t sentSize = serial->write((uint8_t *)dataStruct, _size);

    return (_size == sentSize);
};

bool LoRaE22::getDataStruct(const void* dataStruct, size_t _size)
{
    size_t gotSize = serial->readBytes((uint8_t*) dataStruct, _size);

    return (_size == gotSize);
}

int8_t LoRaE22::getRSSIAmbientNoise()
{
    for(size_t i = 0; i<sizeof(Commands::READ_AMBIENT_RSSI); i++){
        serial->write(Commands::READ_AMBIENT_RSSI[i]);
    }

    return 0;
}

int8_t LoRaE22::getRSSILastRX()
{
    for(size_t i = 0; i<sizeof(Commands::READ_RSSI); i++){
        serial->write(Commands::READ_RSSI[i]);
    }

    return 0;
}



////////////////////////////////////////// Form Config Register Bytes //////////////////////////////////////////////

// REG0 in datasheet
uint8_t LoRaE22::formSerialConfigByte()
{
    uint8_t newByte = ( 
                      (radioConfig.serialSpeed << 5) 
                    | (radioConfig.parityConfig << 3) 
                    | (radioConfig.airDataRate << 0)
                    );
    return newByte;
};

// REG1 in datasheet
uint8_t LoRaE22::formRadioConfigByte()
{
    uint8_t newByte = (
                      (radioConfig.packetSize << 6)
                    | (radioConfig.ambientRSSIEnabled << 5)
                    | (radioConfig.txPower << 0)
                    );
    return newByte;
};

// REG2 in datasheet
uint8_t LoRaE22::formFrequencyByte()
{
    // this is checked on the public facing set, but we check it again to be safe
    uint offset = (radioConfig.frequency - 220125); // in KHz
    uint8_t offsetCount = (offset / 250); // each offset is a 250KHz channel spacing
    return offsetCount;
};

// REG3 in datasheet
uint8_t LoRaE22::formOptionConfigByte()
{
    uint8_t newByte = (
                      (radioConfig.rssiReadingsEnabled << 7)
                    | (radioConfig.destination << 6)
                    | (radioConfig.relayMode << 5)
                    | (radioConfig.listenBeforeTxEnable << 4)
                    | (radioConfig.worMode << 3)
                    | (radioConfig.worPeriod << 0)
                    );
    return newByte;
};



////////////////////////////////////////// De-Form Config Register Bytes //////////////////////////////////////////////

// REG0 in datasheet
void LoRaE22::deformSerialConfigByte(uint8_t byteIn, RadioConfig *config)
{
    config->serialSpeed = getSerialSpeed(byteIn >> 5);
    config->parityConfig = getParityConfig((byteIn & 0b00011000) >> 3);
    config->airDataRate = getAirDataRate(byteIn & 0b00000111);
};

// REG1 in datasheet
void LoRaE22::deformRadioConfigByte(uint8_t byteIn, RadioConfig *config)
{
    config->packetSize = getPacketSize((byteIn & 0b11000000) >> 6);
    config->ambientRSSIEnabled = getEnableRSSI((byteIn & 0b00100000) >> 5);
    config->txPower = getTransmitPower(byteIn & 0b00000011);
};

// REG2 in datasheet
float LoRaE22::deformFrequencyByte(uint8_t byteIn)
{
    float offsetKHz = byteIn * 250.0;
    return (offsetKHz) + 220125;
};

// REG3 in datasheet
void LoRaE22::deformOptionConfigByte(uint8_t byteIn, RadioConfig *config)
{
    config->rssiReadingsEnabled = getEnableRSSI((byteIn & 0b10000000) >> 7);
    config->destination = getDestination((byteIn & 0b01000000) >> 6);
    config->relayMode = getRelayMode((byteIn & 0b00100000) >> 5);
    config->listenBeforeTxEnable = getListenBeforeTX((byteIn & 0b00010000) >> 4);
    config->worMode = getWORMode((byteIn & 0b00001000) >> 3);
    config->worPeriod = getWORPeriod(byteIn & 0b00000111);
};

////////////////////////////////////////// Read Config ////////////////////////////////////////////////////////////////////////////////////////////

// internal func
ConfigStatus LoRaE22::readConfigRegisters()
{
    ConfigStatus output;
    output.readSuccessfully = true;

    size_t length = 12;
    uint8_t readData[length];

    serial->write(Commands::READ);
    serial->write(ConfigRegisters::AddressHigh);
    serial->write(0x09); // want to read all config registers
    serial->flush();

    delay(5);
    while((size_t)serial->available() < length-1){yield();};
    if((size_t)serial->available() == length-1){
        serial->readBytes((uint8_t*)&readData, sizeof(readData));
    }
    
    // return value should be
    // C1 00 09 AA BB CC DD EE FF GG HH II
    #ifdef DEBUG
    for(size_t i = 0; i<length; i++){
        SerialUSB.printf("%02x ", readData[i]);
    }
    SerialUSB.println();
    #endif

    if(readData[0] != Commands::READ){output.readSuccessfully = false; return output;}; // read failed
    if(readData[1] != RadioConfigTypes::ConfigRegisters::AddressHigh){output.readSuccessfully = false; return output;}; // read failed
    if(readData[2] != 0x09){output.readSuccessfully = false; return output;}; // read failed

    // make us a simpler array
    uint8_t registerValues[9];
    memcpy(&registerValues, &readData[3], sizeof(registerValues));

    // build a new config to compare to
    output.config.address = (registerValues[RadioConfigTypes::ConfigRegisters::AddressHigh] << 8) | (registerValues[RadioConfigTypes::ConfigRegisters::AddressLow] << 0);
    output.config.networkId = registerValues[RadioConfigTypes::ConfigRegisters::NetworkID];
    deformSerialConfigByte(registerValues[RadioConfigTypes::ConfigRegisters::SerialConfigRegister], &output.config);
    output.config.frequency = deformFrequencyByte(registerValues[RadioConfigTypes::ConfigRegisters::FrequencyChannel]);
    deformRadioConfigByte(registerValues[RadioConfigTypes::ConfigRegisters::RadioConfigRegister], &output.config);
    deformOptionConfigByte(registerValues[RadioConfigTypes::ConfigRegisters::OptionConfigRegister], &output.config);
    output.config.encryptionKey = (registerValues[RadioConfigTypes::ConfigRegisters::EncryptionHighByte] << 8) | (registerValues[RadioConfigTypes::ConfigRegisters::EncryptionLowByte] << 0);

    return output;
}

/// @brief Checks that config is the same as in memory. Place into programming mode before running this!
/// @return 0 if read successfully and matches. 1 if does not match. -1 if read failed
int8_t LoRaE22::checkConfigMatches()
{
    ConfigStatus readConfig = readConfigRegisters();
    if(!readConfig.readSuccessfully){return -1;};

    // readConfig.config.print();

    if(readConfig.config == radioConfig){
        return 0; // read successfully and config matches
    }
    return 1; // config did not match, read successfully
};

/// @brief Reads config from radio module into memory memory. Place into programming mode before running this!
/// @return true if read successfully. false if failed.
bool LoRaE22::readConfigIntoMemory()
{
    ConfigStatus readConfig = readConfigRegisters();
    if(!readConfig.readSuccessfully){return false;};

    radioConfig = readConfig.config;
    return true;
};

// place into programming mode before running this
// returns true if read correctly, false if not
bool LoRaE22::readProductInfo()
{
    for(int i=0; i<7; i++){
        productInfo[i] = 0;
    }

    serial->write(Commands::READ);
    serial->write(RadioConfigTypes::ConfigRegisters::ProductIDStartByte); // start address
    serial->write(0x07); // want to read all 7 bytes

    uint8_t readData[10];
    serial->readBytes((uint8_t*)&readData, sizeof(readData));

    // return value should be
    // C1 80 07 AA BB CC DD EE FF GG
    if(readData[0] != Commands::READ){return false;};
    if(readData[1] != RadioConfigTypes::ConfigRegisters::ProductIDStartByte){return false;};
    // if(readData[2] != 0x07){return false;};

    // if we pass all those checks, copy our data into our productInfo array
    memcpy(&productInfo, &readData[3], sizeof(productInfo));

    return true;
};


////////////////////////////////////////// Write Config ////////////////////////////////////////////////////////////////////////////////////////////

// build byte buffer of config bytes before sending
size_t LoRaE22::buildConfigBuffer(uint8_t* buffer)
{
    // build config buffer
    uint8_t length = 9;
    buffer[RadioConfigTypes::ConfigRegisters::AddressHigh] = (radioConfig.address >> 8) & 0xFF;
    buffer[RadioConfigTypes::ConfigRegisters::AddressLow] = radioConfig.address & 0xFF;
    buffer[RadioConfigTypes::ConfigRegisters::NetworkID] = radioConfig.networkId;
    buffer[RadioConfigTypes::ConfigRegisters::SerialConfigRegister] = formSerialConfigByte();
    buffer[RadioConfigTypes::ConfigRegisters::RadioConfigRegister] = formRadioConfigByte();
    buffer[RadioConfigTypes::ConfigRegisters::FrequencyChannel] = formFrequencyByte();
    buffer[RadioConfigTypes::ConfigRegisters::OptionConfigRegister] = formOptionConfigByte();
    buffer[RadioConfigTypes::ConfigRegisters::EncryptionHighByte] = (radioConfig.encryptionKey >> 8) & 0xFF;
    buffer[RadioConfigTypes::ConfigRegisters::EncryptionLowByte] = radioConfig.encryptionKey & 0xFF;
    
    return length;
};

// place into programming mode before running this
// this will save the config into the flash of the radio module and will persist through power cycles
bool LoRaE22::writeConfigPersistent(uint8_t* buffer, uint8_t length)
{
    // write our data out
    serial->write(Commands::WRITE_PERMANENT);
    serial->write((uint8_t)RadioConfigTypes::ConfigRegisters::AddressHigh); // our starting address
    serial->write(length); // length we will write
    serial->write(buffer, length);
    serial->flush(); // ensure we block until the write is done

    // read back our data
    size_t lengthRead = 12;
    uint8_t readData[lengthRead];

    #ifdef DEBUG
    SerialUSB.println("config to write:");
    for(size_t i = 0; i<length; i++){
        SerialUSB.printf("%02x ", buffer[i]);
    }
    SerialUSB.println();
    #endif


    delay(10);
    // while(serial->available()){
        // SerialUSB.println(serial->read());
    // }

    while(serial->available() < length-1){yield();digitalToggle(6);};
    while(serial->available() >= length-1){
        serial->readBytes((uint8_t*)&readData, sizeof(readData));
    }
    
    // return value should be
    // C1 00 09 AA BB CC DD EE FF GG HH II
    #ifdef DEBUG
    for(size_t i = 0; i<lengthRead; i++){
        SerialUSB.printf("%02x ", readData[i]);
    }
    SerialUSB.println();
    #endif

    if(readData[0] != Commands::READ){return false;}; // read failed
    if(readData[1] != RadioConfigTypes::ConfigRegisters::AddressHigh){return false;}; // read failed
    if(readData[2] != 0x09){return false;}; // read failed
    // check if any of the written data is incorrect
    for(size_t i = 0; i<length; i++){
        if(readData[i+3] != buffer[i]){return false;};
    }

    return true;
};


// place into programming mode before running this
// this will NOT save the config into the flash of the radio module and will NOT persist through power cycles
bool LoRaE22::writeConfigTemporary(uint8_t* buffer, uint8_t length)
{
    // write our data out
    serial->write(Commands::WRITE_TEMPORARY);
    serial->write((uint8_t)RadioConfigTypes::ConfigRegisters::AddressHigh); // our starting address
    serial->write(length); // length we will write
    serial->write(buffer, length);
    serial->flush(); // ensure we block until the write is done

    return true;
}

bool LoRaE22::remoteWriteConfigPersistent(uint8_t* buffer, uint8_t length)
{
    for(size_t i = 0; i<sizeof(Commands::REMOTE_PREAMBLE); i++){
        serial->write(Commands::REMOTE_PREAMBLE[i]);
    }
    return writeConfigPersistent(buffer, length);
}

bool LoRaE22::remoteWriteConfigTemporary(uint8_t* buffer, uint8_t length)
{
    for(size_t i = 0; i<sizeof(Commands::REMOTE_PREAMBLE); i++){
        serial->write(Commands::REMOTE_PREAMBLE[i]);
    }
    return writeConfigTemporary(buffer, length);
};

 // these were written by ChatGPT...
// --- ParityConfig ---
enum ParityConfig LoRaE22::getParityConfig(int input) {
    switch (input) {
        case Parity_8N1: return Parity_8N1;
        case Parity_8O1: return Parity_8O1;
        case Parity_8E1: return Parity_8E1;
    }
    return Parity_8N1;
};

// --- SerialSpeeds ---
enum SerialSpeeds LoRaE22::getSerialSpeed(int input) {
    switch (input) {
        case BAUD_1200:   return BAUD_1200;
        case BAUD_2400:   return BAUD_2400;
        case BAUD_4800:   return BAUD_4800;
        case BAUD_9600:   return BAUD_9600;
        case BAUD_19200:  return BAUD_19200;
        case BAUD_38400:  return BAUD_38400;
        case BAUD_57600:  return BAUD_57600;
        case BAUD_115200: return BAUD_115200;
    }
    return BAUD_9600;
};

// --- AirDataRate ---
enum AirDataRate LoRaE22::getAirDataRate(int input) {
    switch (input) {
        case 0b000:
        case 0b001:
        case RATE_2400: return RATE_2400;
        case RATE_4800: return RATE_4800;
        case RATE_9600: return RATE_9600;
        case RATE_15600:
        case 0b111: return RATE_15600;
    }
    return RATE_2400;
};

// --- PacketSize ---
enum PacketSize LoRaE22::getPacketSize(int input) {
    switch (input) {
        case SIZE_240: return SIZE_240;
        case SIZE_128: return SIZE_128;
        case SIZE_64:  return SIZE_64;
        case SIZE_32:  return SIZE_32;
    }
    return SIZE_240;
};

// --- EnableRSSIReadings ---
enum EnableRSSIReadings LoRaE22::getEnableRSSI(int input) {
    if(input > 0){
        return EnableRSSIReadings::Enabled;
    }
    return EnableRSSIReadings::Disabled;
};

// --- TransmitPower ---
enum TransmitPower LoRaE22::getTransmitPower(int input) {
    switch (input) {
        case dBm33: return dBm33;
        case dBm30: return dBm30;
        case dBm27: return dBm27;
        case dBm24: return dBm24;
    }
    return dBm33;
};

// --- Destination ---
enum Destination LoRaE22::getDestination(int input) {
    switch (input) {
        case Broadcast: return Broadcast;
        case Unicast:   return Unicast;
    }
    return Broadcast;
};

// --- RelayMode ---
enum RelayMode LoRaE22::getRelayMode(int input) {
    switch (input) {
        case RelayDisabled: return RelayMode::RelayDisabled;
        case RelayEnabled:  return RelayMode::RelayEnabled;
    }
    return RelayDisabled;
};

// --- EnableListenBeforeTX ---
enum EnableListenBeforeTX LoRaE22::getListenBeforeTX(int input) {
    switch (input) {
        case LBTDisabled: return EnableListenBeforeTX::LBTDisabled;
        case LBTEnabled:  return EnableListenBeforeTX::LBTEnabled;
    }
    return LBTDisabled;
};

// --- WakeOnReceiveMode ---
enum WakeOnReceiveMode LoRaE22::getWORMode(int input) {
    switch (input) {
        case NormalWOR:     return WakeOnReceiveMode::NormalWOR;
        case ListenOnly: return WakeOnReceiveMode::ListenOnly;
    }
    return NormalWOR;
};

// --- WakeOnReceiveListenPeriod ---
enum WakeOnReceiveListenPeriod LoRaE22::getWORPeriod(int input) {
    switch (input) {
        case TIME_500:  return TIME_500;
        case TIME_1000: return TIME_1000;
        case TIME_1500: return TIME_1500;
        case TIME_2000: return TIME_2000;
        case TIME_2500: return TIME_2500;
        case TIME_3000: return TIME_3000;
        case TIME_3500: return TIME_3500;
        case TIME_4000: return TIME_4000;
    }
    return TIME_500;
};  
