#pragma once

#include <Arduino.h>

struct Message
{
    alignas(8) uint8_t data[240]; // 240 is max data size
    uint8_t length; // 240 < 255
};

// Turns the radio's received byte stream back into the messages that
// LoRaE22::sendMessage() framed as <callsign><length byte><payload>, and
// queues them until they are read.
//
// Bytes are parsed as they are pushed, by a small state machine:
//   Callsign: matching the callsign, byte by byte. A mismatch starts over.
//   Length:   the next byte is the payload length (1..240).
//   Payload:  that many bytes, then the message is queued.
// Anything that isn't a valid frame (noise, a frame cut short by a lost
// packet) is skipped until the next callsign.
class RxBuffer
{
    public:
        RxBuffer(size_t messageCapacity, const char* Callsign) :
            callsign(Callsign), callsignLength(strlen(Callsign)), msgArrSize(messageCapacity)
        {
            msgArr = new Message[msgArrSize];
        };

        // Parses one received byte. Returns false if it completed a message
        // but the queue was full, so the message was dropped.
        bool pushByte(uint8_t byte)
        {
            switch(state){
                case State::Callsign:
                    if(byte == (uint8_t)callsign[matched]){
                        matched++;
                    }
                    else{
                        // this byte may itself start a new callsign
                        matched = (byte == (uint8_t)callsign[0]) ? 1 : 0;
                    }
                    if(matched == callsignLength){
                        matched = 0;
                        state = State::Length;
                    }
                    break;

                case State::Length:
                    if(byte == 0 || byte > sizeof(newMsg.data)){
                        state = State::Callsign; // not a valid frame, resync
                        break;
                    }
                    newMsg.length = byte;
                    msgIdx = 0;
                    state = State::Payload;
                    break;

                case State::Payload:
                    newMsg.data[msgIdx++] = byte;
                    if(msgIdx == newMsg.length){
                        state = State::Callsign;
                        return pushMessage(newMsg);
                    }
                    break;
            }
            return true;
        };

        bool isMsgEmpty(){return (msgSize == 0);};
        bool isMsgFull(){return (msgSize == msgArrSize);};
        size_t messageCount(){return msgSize;};
        // messages dropped because the queue was full
        uint32_t droppedCount(){return dropped;};

        // oldest message. Only valid if !isMsgEmpty()
        const Message& getFrontMessage(){return msgArr[msgFront];};

        void popMessage()
        {
            if(msgSize == 0){return;};
            msgFront = (msgFront + 1) % msgArrSize;
            msgSize--;
        };

    private:
        enum class State { Callsign, Length, Payload };

        const char* callsign;
        size_t callsignLength;

        State state = State::Callsign;
        size_t matched = 0;   // callsign bytes matched so far
        uint8_t msgIdx = 0;   // payload bytes received so far (< 240)
        Message newMsg;

        Message* msgArr;
        size_t msgArrSize;
        size_t msgFront = 0; // oldest
        size_t msgSize = 0;
        uint32_t dropped = 0;

        bool pushMessage(const Message& msg)
        {
            if(msgSize == msgArrSize){
                dropped++;
                return false;
            };
            size_t back = (msgFront + msgSize) % msgArrSize;
            msgArr[back] = msg;
            msgSize++;
            return true;
        };
};
