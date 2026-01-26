#include "protocol.hpp"

PacketParser::PacketParser() {
    reset();
}

void PacketParser::reset() {
    state = PacketState::WAIT_STX;
    expectedLength = 0;
    receivedBytes = 0;
    dataIndex = 0;
    command = 0;
    calculatedXor = 0;
    for (uint16_t i = 0; i < PROTOCOL_MAX_PACKET_SIZE; ++i) {
        buffer[i] = 0;
    }
}

bool PacketParser::processByte(uint8_t byte) {
    switch (state) {
        case PacketState::WAIT_STX:
            if (byte == PROTOCOL_STX) {
                state = PacketState::WAIT_LENGTH_H;
                receivedBytes = 1;
            }
            break;

        case PacketState::WAIT_LENGTH_H:
            expectedLength = static_cast<uint16_t>(byte) << 8;
            calculatedXor = byte;
            state = PacketState::WAIT_LENGTH_L;
            receivedBytes = 2;
            break;

        case PacketState::WAIT_LENGTH_L:
            expectedLength |= byte;
            calculatedXor ^= byte;
            receivedBytes = 3;

            if (expectedLength < PROTOCOL_MIN_PACKET_SIZE ||
                expectedLength > PROTOCOL_MAX_PACKET_SIZE) {
                reset();
                return false;
            }
            state = PacketState::WAIT_CMD;
            break;

        case PacketState::WAIT_CMD:
            command = byte;
            calculatedXor ^= byte;
            receivedBytes = 4;
            dataIndex = 0;

            if (expectedLength == PROTOCOL_MIN_PACKET_SIZE) {
                state = PacketState::WAIT_XOR;
            } else {
                state = PacketState::WAIT_DATA;
            }
            break;

        case PacketState::WAIT_DATA:
            buffer[dataIndex++] = byte;
            calculatedXor ^= byte;
            receivedBytes++;

            if (receivedBytes == expectedLength - 1) {
                state = PacketState::WAIT_XOR;
            }
            break;

        case PacketState::WAIT_XOR:
            receivedBytes++;
            if (calculatedXor == byte) {
                state = PacketState::PACKET_READY;
                return true;
            } else {
                reset();
                return false;
            }
            break;

        case PacketState::PACKET_READY:
            break;
    }

    return false;
}

bool PacketParser::isComplete() const {
    return state == PacketState::PACKET_READY;
}

uint16_t PacketParser::getDataLength() const {
    if (expectedLength <= PROTOCOL_MIN_PACKET_SIZE) {
        return 0;
    }
    return expectedLength - PROTOCOL_MIN_PACKET_SIZE;
}

uint8_t calculateXor(const uint8_t* data, uint16_t length) {
    uint8_t xorValue = 0;
    for (uint16_t i = 0; i < length; ++i) {
        xorValue ^= data[i];
    }
    return xorValue;
}
