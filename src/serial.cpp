#include "serial.hpp"
#include "constants.hpp"
#include "../system/include/cmsis/stm32f4xx.h"

static void initUSART2()
{
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    USART2->CR1 = 0;
    USART2->CR2 = 0;
    USART2->CR3 = 0;

    USART2->BRR = 0x8B;  // 115200
    USART2->CR1 |= (USART_CR1_TE | USART_CR1_RE);
    USART2->CR1 |= USART_CR1_UE;
}

static void initUART4()
{
    RCC->APB1ENR |= RCC_APB1ENR_UART4EN;
    UART4->CR1 = 0;
    UART4->CR2 = 0;
    UART4->CR3 = 0;

    UART4->BRR = 0x8B;  // 115200
    UART4->CR1 |= (USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE);
    UART4->CR1 |= USART_CR1_UE;

    NVIC_EnableIRQ(UART4_IRQn);
    NVIC_SetPriority(UART4_IRQn, 5);
}

void initSerial()
{
    initUSART2();
    initUART4();
}

void sendByte2PC(uint8_t data)
{
    while (!(UART4->SR & USART_SR_TXE));
    UART4->DR = data;
}

void sendPacket(uint8_t responseCmd, const uint8_t* data, uint16_t dataLen) {
    uint16_t totalLength = PROTOCOL_MIN_PACKET_SIZE + dataLen;
    uint8_t lengthH = static_cast<uint8_t>(totalLength >> 8);
    uint8_t lengthL = static_cast<uint8_t>(totalLength & 0xFF);

    uint8_t xorValue = lengthH ^ lengthL ^ responseCmd;
    for (uint16_t i = 0; i < dataLen; ++i) {
        xorValue ^= data[i];
    }

    sendByte2PC(PROTOCOL_STX);
    sendByte2PC(lengthH);
    sendByte2PC(lengthL);
    sendByte2PC(responseCmd);

    for (uint16_t i = 0; i < dataLen; ++i) {
        sendByte2PC(data[i]);
    }

    sendByte2PC(xorValue);
}

void sendErrorPacket(uint8_t errorCode) {
    sendPacket(Response::ERROR, &errorCode, 1);
}

void sendVersionResponse() {
    uint8_t version = FIRMWARE_VERSION;
    sendPacket(Response::VERSION, &version, 1);
}

void sendStatusResponse(uint16_t activeMotors, uint16_t completedMotors, uint16_t statusPins) {
    uint8_t data[6];
    data[0] = static_cast<uint8_t>(activeMotors & 0xFF);
    data[1] = static_cast<uint8_t>((activeMotors >> 8) & 0xFF);
    data[2] = static_cast<uint8_t>(completedMotors & 0xFF);
    data[3] = static_cast<uint8_t>((completedMotors >> 8) & 0xFF);
    data[4] = static_cast<uint8_t>(statusPins & 0xFF);
    data[5] = static_cast<uint8_t>((statusPins >> 8) & 0xFF);
    sendPacket(Response::STATUS, data, 6);
}

void sendStopResponse(uint8_t result) {
    sendPacket(Response::STOP, &result, 1);
}

void sendMoveResponse(uint8_t result) {
    sendPacket(Response::MOVE, &result, 1);
}