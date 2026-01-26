#pragma once

#include <cstdint>

void initSerial();
void sendByte2PC(uint8_t data);
void sendPacket(uint8_t responseCmd, const uint8_t* data, uint16_t dataLen);
void sendErrorPacket(uint8_t errorCode);
void sendVersionResponse();
void sendStatusResponse(uint16_t activeMotors, uint16_t completedMotors);
void sendStopResponse(uint8_t result);
void sendMoveResponse(uint8_t result);
