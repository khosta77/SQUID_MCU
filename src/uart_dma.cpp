#include "uart_dma.hpp"
#include "protocol.hpp"
#include "../system/include/cmsis/stm32f4xx.h"

UartDma g_uartDma;

extern PacketParser g_packetParser;
extern volatile bool g_packetReady;

void UartDma::init() {
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
    RCC->APB1ENR |= RCC_APB1ENR_UART4EN;

    UART4->CR1 = 0;
    UART4->CR2 = 0;
    UART4->CR3 = 0;

    UART4->BRR = 0x8B;  // 115200 @ 16MHz HSI

    UART4->CR3 |= USART_CR3_DMAR | USART_CR3_DMAT;
    UART4->CR1 |= USART_CR1_IDLEIE;
    UART4->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;

    NVIC_EnableIRQ(UART4_IRQn);
    NVIC_SetPriority(UART4_IRQn, 6);

    DMA1_Stream2->CR &= ~DMA_SxCR_EN;
    while (DMA1_Stream2->CR & DMA_SxCR_EN);

    DMA1_Stream2->CR = 0;
    DMA1_Stream2->CR |= (4U << DMA_SxCR_CHSEL_Pos);
    DMA1_Stream2->CR |= DMA_SxCR_MINC;
    DMA1_Stream2->CR |= DMA_SxCR_CIRC;
    DMA1_Stream2->CR |= DMA_SxCR_HTIE | DMA_SxCR_TCIE;
    DMA1_Stream2->PAR = reinterpret_cast<uint32_t>(&UART4->DR);
    DMA1_Stream2->M0AR = reinterpret_cast<uint32_t>(_rxBuffer);
    DMA1_Stream2->NDTR = UART_DMA_RX_BUFFER_SIZE;

    NVIC_EnableIRQ(DMA1_Stream2_IRQn);
    NVIC_SetPriority(DMA1_Stream2_IRQn, 5);

    DMA1_Stream4->CR &= ~DMA_SxCR_EN;
    while (DMA1_Stream4->CR & DMA_SxCR_EN);

    DMA1_Stream4->CR = 0;
    DMA1_Stream4->CR |= (4U << DMA_SxCR_CHSEL_Pos);
    DMA1_Stream4->CR |= DMA_SxCR_MINC;
    DMA1_Stream4->CR |= DMA_SxCR_DIR_0;
    DMA1_Stream4->CR |= DMA_SxCR_TCIE;
    DMA1_Stream4->PAR = reinterpret_cast<uint32_t>(&UART4->DR);

    NVIC_EnableIRQ(DMA1_Stream4_IRQn);
    NVIC_SetPriority(DMA1_Stream4_IRQn, 7);
}

void UartDma::startRx() {
    _rxHead = 0;
    _rxTail = 0;
    _rxPending = false;

    DMA1->LIFCR = DMA_LIFCR_CTCIF2 | DMA_LIFCR_CHTIF2 | DMA_LIFCR_CTEIF2;
    DMA1_Stream2->CR |= DMA_SxCR_EN;
}

void UartDma::sendPacket(const uint8_t* data, uint16_t length) {
    if (length > UART_DMA_TX_BUFFER_SIZE || length == 0) {
        return;
    }

    while (_txBusy);

    for (uint16_t i = 0; i < length; ++i) {
        _txBuffer[i] = data[i];
    }

    _txBusy = true;

    DMA1_Stream4->CR &= ~DMA_SxCR_EN;
    while (DMA1_Stream4->CR & DMA_SxCR_EN);

    DMA1->HIFCR = DMA_HIFCR_CTCIF4 | DMA_HIFCR_CHTIF4 | DMA_HIFCR_CTEIF4;

    DMA1_Stream4->M0AR = reinterpret_cast<uint32_t>(_txBuffer);
    DMA1_Stream4->NDTR = length;
    DMA1_Stream4->CR |= DMA_SxCR_EN;
}

uint16_t UartDma::getRxDataLength() const {
    uint16_t currentPos = UART_DMA_RX_BUFFER_SIZE - DMA1_Stream2->NDTR;

    if (currentPos >= _rxTail) {
        return currentPos - _rxTail;
    } else {
        return UART_DMA_RX_BUFFER_SIZE - _rxTail + currentPos;
    }
}

void UartDma::processRxBuffer(uint16_t startPos, uint16_t endPos) {
    uint16_t pos = startPos;

    while (pos != endPos) {
        uint8_t byte = _rxBuffer[pos];

        if (g_packetParser.processByte(byte)) {
            g_packetReady = true;
        }

        pos = (pos + 1) % UART_DMA_RX_BUFFER_SIZE;
    }

    _rxTail = endPos;
}

void UartDma::processRxData() {
    uint16_t currentPos = UART_DMA_RX_BUFFER_SIZE - DMA1_Stream2->NDTR;

    if (currentPos != _rxTail) {
        processRxBuffer(_rxTail, currentPos);
    }

    _rxPending = false;
}

void UartDma::handleDmaRxIrq() {
    if (DMA1->LISR & DMA_LISR_HTIF2) {
        DMA1->LIFCR = DMA_LIFCR_CHTIF2;
        _rxPending = true;
        GPIOD->ODR ^= GPIO_ODR_OD14;
    }

    if (DMA1->LISR & DMA_LISR_TCIF2) {
        DMA1->LIFCR = DMA_LIFCR_CTCIF2;
        _rxPending = true;
        GPIOD->ODR ^= GPIO_ODR_OD15;
    }
}

void UartDma::handleDmaTxIrq() {
    if (DMA1->HISR & DMA_HISR_TCIF4) {
        DMA1->HIFCR = DMA_HIFCR_CTCIF4;
        DMA1_Stream4->CR &= ~DMA_SxCR_EN;
        _txBusy = false;
    }
}

void UartDma::handleUartIdleIrq() {
    if (UART4->SR & USART_SR_IDLE) {
        volatile uint32_t tmp = UART4->SR;
        tmp = UART4->DR;
        (void)tmp;

        _rxPending = true;
        GPIOD->ODR ^= GPIO_ODR_OD13;
    }
}
