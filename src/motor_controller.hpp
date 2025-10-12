#pragma once

#include <stdint.h>
#include "constants.hpp"
#include "motor_settings.hpp"
#include "serial.hpp"

// Макросы обработки команд протокола

#define GET_MOTOR_COUNT(command) ((command) & CMD_MOTOR_COUNT_MASK)
#define IS_SYNC_COMMAND(command) (((command) & CMD_SYNC_MASK) == CMD_SYNC_MASK)
#define IS_ASYNC_COMMAND(command) (((command) & CMD_ASYNC_MASK) == CMD_ASYNC_MASK)
#define IS_VERSION_COMMAND(command) (((command) & CMD_VERSION_MASK) == CMD_VERSION_MASK)
#define VALIDATE_MOTOR_COUNT(count) ((count) >= 1 && (count) <= MAX_MOTORS)

// Глобальные переменные для отслеживания состояния моторов
extern volatile uint16_t activeMotors;        // Битовое поле активных моторов
extern volatile uint16_t completedMotors;     // Битовое поле завершенных моторов
extern volatile bool emergencyStop;           // Флаг аварийной остановки
extern volatile uint8_t currentMotorCount;    // Количество моторов в текущей команде
extern volatile uint16_t syncMotorBuffer;     // Битовое поле моторов для синхронного запуска

// Функции инициализации
void GPIO_init();
void USART_init();
void DMA_init();
void SystemClock_HSI_Config(void);
void initMotorInterrupts();
void initEndstopInterrupts();

/* 
 * @brief Перезапуск DMA
 * @details В случае, если прозошла ошибка, либо выход за таймаут, перезапускает DMA для приема данных моторов
 */
void stopDMAStream2();

// Функции управления моторами
void configureMotor(uint8_t motorNumber, const MotorSettings& params);

// Функции обработки команд
void processCommand(uint8_t command);

// Функция ожидания данных моторов
void waitForMotorDataAndProcess(uint8_t command, uint8_t motorCount);

// Функции для работы с драйверами
void send2driver(const uint8_t *frame);

// Функции обработки режимов
void processSyncMode(uint8_t motorCount);
void processAsyncMode(uint8_t motorCount);

// Функции управления моторами
void stopAllMotors();
void clear_usart4_rx_array();
void enableAllMotorsByDefault();

// Функции состояния системы
void emergencyStopAllMotors();
bool checkAllMotorsCompleted();
void resetMotorStates();

// Обработчики прерываний
extern "C" void DMA1_Stream6_IRQHandler(void);
extern "C" void DMA1_Stream2_IRQHandler(void);
extern "C" void EXTI0_IRQHandler(void);
extern "C" void EXTI1_IRQHandler(void);
extern "C" void EXTI2_IRQHandler(void);
extern "C" void EXTI3_IRQHandler(void);
extern "C" void EXTI4_IRQHandler(void);
extern "C" void EXTI9_5_IRQHandler(void);
extern "C" void EXTI15_10_IRQHandler(void);
