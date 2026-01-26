#pragma once

#include <cstdint>
#include "constants.hpp"

enum class PacketState : uint8_t {
    WAIT_STX,
    WAIT_LENGTH_H,
    WAIT_LENGTH_L,
    WAIT_CMD,
    WAIT_DATA,
    WAIT_XOR,
    PACKET_READY
};

struct PacketParser {
    PacketState state;
    uint16_t expectedLength;
    uint16_t receivedBytes;
    uint16_t dataIndex;
    uint8_t command;
    uint8_t calculatedXor;
    uint8_t buffer[PROTOCOL_MAX_PACKET_SIZE];

    PacketParser();
    void reset();
    bool processByte(uint8_t byte);
    bool isComplete() const;

    uint8_t getCommand() const { return command; }
    const uint8_t* getData() const { return buffer; }
    uint16_t getDataLength() const;
};

uint8_t calculateXor(const uint8_t* data, uint16_t length);
