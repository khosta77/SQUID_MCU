#include "constants.hpp"

uint8_t usart4_mrk = 0x00;
uint8_t usart4_rx_array[256];
uint8_t usart2_mrk = 0xFF;

// Переменные состояния для двухэтапного протокола
volatile bool waitingForMotorData = false;  // Флаг ожидания данных моторов
volatile uint16_t expectedDataSize = 0;     // Ожидаемый размер данных
volatile uint32_t timeoutCounter = 0;       // Счетчик таймаута
volatile bool timeoutOccurred = false;       // Флаг таймаута

// Глобальные переменные для отслеживания состояния моторов
volatile uint16_t activeMotors = 0;        // Битовое поле активных моторов
volatile uint16_t completedMotors = 0;     // Битовое поле завершенных моторов
volatile bool emergencyStop = false;       // Флаг аварийной остановки
volatile uint8_t currentMotorCount = 0;    // Количество моторов в текущей команде
volatile uint16_t syncMotorBuffer = 0;     // Битовое поле моторов для синхронного запуска