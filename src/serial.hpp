#pragma once

#include <cstdint>

/*
 * @brief Инициализация USART2 и UART4
 */
inline void initSerial();

/*
 * @brief Отправка байта на ПК
 * @param data Байт для отправки
 */
void sendByte2PC(const uint8_t data);
