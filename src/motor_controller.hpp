#pragma once

#include <stdint.h>
#include "constants.hpp"
#include "motor_settings.hpp"

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

// Функции управления моторами
void configureMotor(uint8_t motorNumber, const MotorSettings& params);

// Функции обработки команд
void processCommand(uint8_t command);

// Глобальная функция для отправки ответов
void sendResponse(uint8_t response);

// Функции для работы с драйверами
void send2driver(const uint8_t *frame);

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
