#pragma once

#include <cstdint>

// Система версионирования прошивки
constexpr uint8_t FIRMWARE_VERSION_MAJOR = 1;
constexpr uint8_t FIRMWARE_VERSION_MINOR = 0;
constexpr uint8_t FIRMWARE_VERSION = (FIRMWARE_VERSION_MAJOR << 4) | FIRMWARE_VERSION_MINOR;

// ============================================================================
// Новый пакетный протокол
// ============================================================================
// Формат: [STX] [Length_H] [Length_L] [Cmd] [Data...] [XOR]
// - STX не участвует в XOR
// - Length = полная длина пакета (от STX до XOR)
// - XOR = xor от Length_H до последнего байта Data
// ============================================================================

constexpr uint8_t PROTOCOL_STX = 0x02;
constexpr uint16_t PROTOCOL_MIN_PACKET_SIZE = 5;
constexpr uint16_t PROTOCOL_MAX_PACKET_SIZE = 256;
constexpr uint8_t PROTOCOL_HEADER_SIZE = 4;

// Коды команд (TX от PC к MCU)
namespace Cmd {
    constexpr uint8_t VERSION    = 0x01;
    constexpr uint8_t STATUS     = 0x02;
    constexpr uint8_t STOP       = 0x03;
    constexpr uint8_t SYNC_MOVE  = 0x10;
    constexpr uint8_t ASYNC_MOVE = 0x11;
}

// Коды ответов (RX от MCU к PC)
namespace Response {
    constexpr uint8_t VERSION    = 0x81;
    constexpr uint8_t STATUS     = 0x82;
    constexpr uint8_t STOP       = 0x83;
    constexpr uint8_t MOVE       = 0x90;
    constexpr uint8_t ERROR      = 0xFF;
}

// Коды ошибок
namespace Error {
    constexpr uint8_t INVALID_COMMAND       = 0x01;
    constexpr uint8_t INVALID_PACKET_LENGTH = 0x02;
    constexpr uint8_t XOR_CHECKSUM_ERROR    = 0x03;
    constexpr uint8_t INVALID_MOTOR_COUNT   = 0x04;
    constexpr uint8_t MOTOR_PARAM_ERROR     = 0x05;
    constexpr uint8_t EMERGENCY_STOP        = 0x0B;
    constexpr uint8_t TIMEOUT               = 0x0D;
}

// Коды результата
namespace Result {
    constexpr uint8_t SUCCESS = 0x00;
    constexpr uint8_t BUSY    = 0x01;
}

// Максимальное количество моторов
constexpr uint8_t MAX_MOTORS = 10;

// Внешние переменные из main.cpp
extern uint8_t usart4_rx_array[256];
extern uint8_t usart2_mrk;
extern uint8_t usart4_mrk;

// Внешние переменные состояния из main.cpp
extern volatile bool waitingForMotorData;
extern volatile uint16_t expectedDataSize;
extern volatile uint32_t timeoutCounter;
extern volatile bool timeoutOccurred;

// Глобальные переменные для отслеживания состояния моторов
extern volatile uint16_t activeMotors;        // Битовое поле активных моторов
extern volatile uint16_t completedMotors;     // Битовое поле завершенных моторов
extern volatile bool emergencyStop;           // Флаг аварийной остановки
extern volatile uint8_t currentMotorCount;    // Количество моторов в текущей команде
extern volatile uint16_t syncMotorBuffer;     // Битовое поле моторов для синхронного запуска
