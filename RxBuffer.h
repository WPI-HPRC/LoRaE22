#pragma once

#include <Arduino.h>

struct Message
{
    alignas(8) uint8_t data[240]; // 240 is max data size
    uint8_t length; // 240 < 255
};

class RxBuffer 
{
    public:
        RxBuffer(size_t byteCapacity, size_t messageCapacity, const char* Callsign) : 
            callsign(Callsign), byteArrSize(byteCapacity), msgArrSize(messageCapacity), front(0), size(0)
        {
            byteArr = new uint8_t[byteArrSize];
            for(size_t i = 0; i<byteArrSize;i++){byteArr[i] = 0;};
            msgArr = new Message[msgArrSize];
            callsignLength = strlen(callsign);
        };

        bool pushByte(uint8_t byte)
        {
            if(size == byteArrSize){return false;};
            size_t back = (front + size) % byteArrSize;
            byteArr[back] = byte;
            size++;

            process();

            return true;
        };

        bool isEmpty(){return (size == 0);};
        bool isFull(){return (size == byteArrSize);};

        uint8_t popByte()
        {
            uint8_t frontByte = byteArr[front];
            pop();
            return frontByte;
        };

        uint8_t getFront(){return byteArr[front];};
        uint8_t getBack(){return byteArr[((front + size) % byteArrSize)];};

        void pop()
        {
            front = (front + 1) %byteArrSize;
            size --;
        }

        bool isMsgEmpty(){return (msgSize == 0);};
        bool isMsgFull(){return (msgSize == msgArrSize);};

        Message getFrontMessage(){return msgArr[msgFront];};
        Message getBackMessage(){return msgArr[((msgFront + msgSize) % msgArrSize)];};

        void popMessage()
        {
            msgFront = (msgFront + 1) % msgArrSize;
            msgSize--;
        };

    private:
        const char* callsign;
        
        
        size_t byteArrSize;
        size_t msgArrSize;

        size_t callsignLength;
        
        size_t front; // oldest, back is newest
        size_t size;

        size_t msgFront; // oldest, back is newest
        size_t msgSize;

        size_t maxMessageSize;

        bool inMessage = false;
        bool possibleNewMsg = false;
        size_t correctCallsignCount;
        uint8_t msgIdx; // will never be > 255, as we are sticking within our 240 byte packets


        uint8_t* byteArr;
        Message* msgArr;

        Message newMsg;
        

        bool pushMessage(Message msg)
        {
            if(size == msgArrSize){return false;};
            size_t back = (msgFront + msgSize) % msgArrSize;
            msgArr[back] = msg;
            msgSize++;

            return true;
        };

        void printByteArr(){
            for(size_t i = 0; i< byteArrSize; i++){
                SerialUSB.printf("%0X ", byteArr[i]);
            }
            SerialUSB.println("");
        }
        

        void process()
        {
            // printByteArr();
            if(!possibleNewMsg){
                if(!isEmpty() && getFront() == callsign[0]){
                    correctCallsignCount++;
                    possibleNewMsg = true;
                    popByte();
                }
            }
            else if(possibleNewMsg){
                if(correctCallsignCount == callsignLength){
                    inMessage = true;
                    correctCallsignCount = 0;
                    msgIdx = 0;
                    newMsg.length = 0;
                }
                if(!isEmpty() && getFront() == callsign[correctCallsignCount]){
                    correctCallsignCount++;
                    popByte();
                }
            }


            if(inMessage){
                
                if(!isEmpty() && possibleNewMsg){ // we just entered
                    possibleNewMsg = false;
                    newMsg.length = popByte();
                }
                else if(!isEmpty() && msgIdx < newMsg.length){
                    newMsg.data[msgIdx] = popByte();
                    // SerialUSB.printf("%0X",msgArr[msgFront].data[0]);
                    msgIdx++;
                }
                if(msgIdx == newMsg.length+1){ // we've reached the end of the message
                    inMessage = false;
                    pushMessage(newMsg);
                    // SerialUSB.println("msg done!");
                    // for(size_t i = 0;i<newMsg.length;i++){
                    //     SerialUSB.printf("%0X ",newMsg.data[i]);
                    // }
                }
                // SerialUSB.println("we found a message!");
                // SerialUSB.print(String(msgIdx) + " " + String(msgArr));
            }
        };

    
};