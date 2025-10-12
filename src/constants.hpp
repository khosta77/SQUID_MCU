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
