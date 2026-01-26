#pragma once

#include <cstdint>

constexpr uint16_t UART_DMA_RX_BUFFER_SIZE = 64;
constexpr uint16_t UART_DMA_TX_BUFFER_SIZE = 64;

class UartDma {
public:
    UartDma() = default;

    void init();

    void startRx();
    void sendPacket(const uint8_t* data, uint16_t length);

    uint16_t getRxDataLength() const;
    const uint8_t* getRxBuffer() const { return _rxBuffer; }

    void processRxData();
    bool hasPendingRxData() const { return _rxPending; }
    void clearRxPending() { _rxPending = false; }

    void handleDmaRxIrq();
    void handleDmaTxIrq();
    void handleUartIdleIrq();

private:
    void processRxBuffer(uint16_t startPos, uint16_t endPos);

    uint8_t _rxBuffer[UART_DMA_RX_BUFFER_SIZE];
    uint8_t _txBuffer[UART_DMA_TX_BUFFER_SIZE];

    volatile uint16_t _rxHead = 0;
    volatile uint16_t _rxTail = 0;
    volatile bool _rxPending = false;
    volatile bool _txBusy = false;
};

extern UartDma g_uartDma;
