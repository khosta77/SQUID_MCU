#pragma once

#include <stdint.h>
#include "constants.hpp"
#include "motor_settings.hpp"
#include "serial.hpp"
#include "protocol.hpp"

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

// Функции обработки команд
void processPacketCommand(const PacketParser& packet);

// Функции для работы с драйверами
void send2driver(const uint8_t *frame);
void clear_usart4_rx_array();

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
