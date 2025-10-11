#pragma once

#include <cstdint>

// Система версионирования прошивки
constexpr uint8_t FIRMWARE_VERSION_MAJOR = 1;
constexpr uint8_t FIRMWARE_VERSION_MINOR = 0;
constexpr uint8_t FIRMWARE_VERSION = (FIRMWARE_VERSION_MAJOR << 4) | FIRMWARE_VERSION_MINOR;

// Константы протокола
constexpr uint8_t CMD_SYNC_MASK = 0x80;         // 0x8N - синхронный режим
constexpr uint8_t CMD_ASYNC_MASK = 0x40;        // 0x4N - асинхронный режим  
constexpr uint8_t CMD_VERSION_MASK = 0x20;      // 0x20 - запрос версии
constexpr uint8_t CMD_MOTOR_COUNT_MASK = 0x0F;  // Младшие 4 бита - количество моторов

// Коды ответов
constexpr uint8_t RESPONSE_READY = 0x00;     // Готовность к приему
constexpr uint8_t RESPONSE_SUCCESS = 0xFF;   // Успешное выполнение

// Максимальное количество моторов
constexpr uint8_t MAX_MOTORS = 10;
