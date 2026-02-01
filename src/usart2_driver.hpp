#pragma once

#include <cstdint>

class Usart2Driver {
public:
    static void send(const uint8_t* data, uint8_t length);
    static void sendByte(uint8_t data);
    static bool hasData();
    static uint8_t readByte();
    static void waitTransmitComplete();
};
