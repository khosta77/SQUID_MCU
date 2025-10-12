#include "serial.hpp"
#include "../system/include/cmsis/stm32f4xx.h"

static void initUSART2()
{
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    USART2->CR1 = 0; // Сбрасываем все биты
    USART2->CR2 = 0; // 1 стоп-бит
    USART2->CR3 = 0;
    
    // USART2 - связь с драйверами моторов
    USART2->BRR = 0x8B;  // 115200
    USART2->CR1 |= (USART_CR1_TE | USART_CR1_RE);
    USART2->CR3 |= (USART_CR3_DMAT);
    USART2->CR1 |= USART_CR1_UE;
}

static void initUART4()
{
    RCC->APB1ENR |= RCC_APB1ENR_UART4EN;
    UART4->CR1 = 0; // Сбрасываем все биты
    UART4->CR2 = 0; // 1 стоп-бит
    UART4->CR3 = 0;

    // UART4 - связь с ПК
    UART4->BRR = 0x8B;  // 115200 
    UART4->CR1 |= (USART_CR1_TE | USART_CR1_RE);
    UART4->CR3 |= USART_CR3_DMAR;  // Только DMA для приема
    UART4->CR1 |= USART_CR1_UE;
}

void initSerial()
{
    initUSART2();
    initUART4();
}

void sendByte2PC(const uint8_t data)
{
    while (!(UART4->SR & USART_SR_TXE));
    UART4->DR = data;
}