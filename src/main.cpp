#include "../system/include/cmsis/stm32f4xx.h"
#include "motor_controller.hpp"
#include "constants.hpp"
#include "gpio.hpp"
#include "protocol.hpp"
#include "motor_simulator.hpp"
#include "uart_dma.hpp"

PacketParser g_packetParser;
volatile bool g_packetReady = false;

void clear_usart4_rx_array() {
    for (uint16_t i = 0; i < 256; i++) {
        usart4_rx_array[i] = 0x00;
    }
}

void stopDMAStream2() {
    DMA1_Stream2->CR &= ~DMA_SxCR_EN;
    while ((DMA1_Stream2->CR & DMA_SxCR_EN) == DMA_SxCR_EN);
}

void DMA_init() {
}

void SystemClock_HSI_Config(void) {
    RCC->CR |= RCC_CR_HSION;
    while (!(RCC->CR & RCC_CR_HSIRDY));

    RCC->CFGR &= ~RCC_CFGR_HPRE;
    RCC->CFGR &= ~RCC_CFGR_PPRE1;
    RCC->CFGR &= ~RCC_CFGR_PPRE2;

    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_HSI;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI);

    SystemCoreClockUpdate();
}

void SysTick_Init(void) {
    SysTick->LOAD = (SystemCoreClock / 1000) - 1;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}

int main(void) {
    SystemClock_HSI_Config();
    clear_usart4_rx_array();
    initGPIO();
    SysTick_Init();

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    GPIOD->MODER |= GPIO_MODER_MODER12_0 | GPIO_MODER_MODER13_0 | GPIO_MODER_MODER14_0 | GPIO_MODER_MODER15_0;
    GPIOD->ODR |= GPIO_ODR_OD12 | GPIO_ODR_OD13 | GPIO_ODR_OD14 | GPIO_ODR_OD15;

    g_uartDma.init();
    g_uartDma.startRx();

    while (1) {
        if (g_uartDma.hasPendingRxData()) {
            g_uartDma.processRxData();
        }

        if (g_packetReady) {
            g_packetReady = false;
            GPIOD->ODR ^= GPIO_ODR_OD12;

            processPacketCommand(g_packetParser);

            g_packetParser.reset();
        }
    }
}

extern "C" void __attribute__((interrupt, used)) DMA1_Stream2_IRQHandler(void) {
    g_uartDma.handleDmaRxIrq();
}

extern "C" void __attribute__((interrupt, used)) DMA1_Stream4_IRQHandler(void) {
    g_uartDma.handleDmaTxIrq();
}

extern "C" void __attribute__((interrupt, used)) UART4_IRQHandler(void) {
    g_uartDma.handleUartIdleIrq();
}

extern "C" void __attribute__((interrupt, used)) SysTick_Handler(void) {
    g_motorSimulator.tick();
}
