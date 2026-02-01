#include "usart2_driver.hpp"
#include "../system/include/cmsis/stm32f4xx.h"

void Usart2Driver::send(const uint8_t* data, uint8_t length) {
    for (uint8_t i = 0; i < length; ++i) {
        sendByte(data[i]);
    }
}

void Usart2Driver::sendByte(uint8_t data) {
    while (!(USART2->SR & USART_SR_TXE));
    USART2->DR = data;
}

bool Usart2Driver::hasData() {
    return (USART2->SR & USART_SR_RXNE) != 0;
}

uint8_t Usart2Driver::readByte() {
    if (!hasData()) {
        return 0;
    }
    return static_cast<uint8_t>(USART2->DR);
}

void Usart2Driver::waitTransmitComplete() {
    while (!(USART2->SR & USART_SR_TC));
}
