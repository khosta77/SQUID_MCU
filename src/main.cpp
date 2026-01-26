#include "../system/include/cmsis/stm32f4xx.h"
#include "motor_controller.hpp"
#include "constants.hpp"
#include "motor_settings.hpp"
#include "gpio.hpp"
#include "serial.hpp"
#include "protocol.hpp"
#include "motor_simulator.hpp"

#include <string.h>

PacketParser g_packetParser;
volatile bool g_packetReady = false;
volatile uint8_t g_rxByte = 0;

void clear_usart4_rx_array()
{
    for(uint16_t i = 0; i < 256; i++) {
        usart4_rx_array[i] = 0x00;
    }
}

void stopDMAStream2() {
    DMA1_Stream2->CR &= ~DMA_SxCR_EN;
    while ((DMA1_Stream2->CR & DMA_SxCR_EN) == DMA_SxCR_EN)
        ;
}

#if 0 // MOTOR_CODE_DISABLED
void theTimeoutWorked()
{
    waitingForMotorData = false;
    sendByte2PC(0x0D);
}

// Функция ожидания данных моторов и их обработки
void waitForMotorDataAndProcess(uint8_t command, uint8_t motorCount)
{
    const uint32_t timeoutLimit = 5000000;
    uint32_t timeoutCounter = 0;

    while ((DMA1_Stream2->CR & DMA_SxCR_EN) == DMA_SxCR_EN)
    {
#if 0
        if (timeoutCounter++ > timeoutLimit)
            break;

        if (emergencyStop)
        {
            emergencyStopAllMotors();
            sendByte2PC(0x0B);
            waitingForMotorData = false;
            return;
        }
#endif
    }

    if (timeoutCounter >= timeoutLimit)
        return theTimeoutWorked();

    if (IS_SYNC_COMMAND(command))
    {
        processSyncMode(motorCount);
    }

    if (IS_ASYNC_COMMAND(command))
    {
        processAsyncMode(motorCount);
    }

    waitingForMotorData = false;
}

void send2driver(const uint8_t *frame)
{
    while (usart2_mrk == 0x00)
        ;
    usart2_mrk = 0x00;
    DMA1_Stream6->M0AR = (uint32_t)frame;
    DMA1_Stream6->NDTR = 12;
    DMA1_Stream6->CR |= DMA_SxCR_EN;
}
#endif // MOTOR_CODE_DISABLED

void DMA_init()
{
#if 0 // MOTOR_CODE_DISABLED - DMA currently not used
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;

    DMA1_Stream6->CR &= ~DMA_SxCR_EN;
    while ((DMA1_Stream6->CR & DMA_SxCR_EN) == DMA_SxCR_EN)
        ;

    DMA1_Stream6->CR |= ((0x4 << 25) | DMA_SxCR_MINC | DMA_SxCR_TCIE);
    DMA1_Stream6->CR &= ~(DMA_SxCR_MSIZE | DMA_SxCR_PSIZE);
    DMA1_Stream6->CR |= (0x01 << 6);
    DMA1_Stream6->NDTR = 12;
    DMA1_Stream6->PAR = (uint32_t)(&USART2->DR);
    NVIC_EnableIRQ(DMA1_Stream6_IRQn);
    NVIC_SetPriority(DMA1_Stream6_IRQn, 7);
#endif
}

void SystemClock_HSI_Config(void)
{
    RCC->CR |= RCC_CR_HSION;
    while (!(RCC->CR & RCC_CR_HSIRDY))
        ;

    RCC->CFGR &= ~RCC_CFGR_HPRE;
    RCC->CFGR &= ~RCC_CFGR_PPRE1;
    RCC->CFGR &= ~RCC_CFGR_PPRE2;

    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_HSI;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI)
        ;
    SystemCoreClockUpdate();
}

void SysTick_Init(void)
{
    SysTick->LOAD = (SystemCoreClock / 1000) - 1;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}

#if 0 // MOTOR_CODE_DISABLED
// Инициализация прерываний для STATUS моторов (PE0-PE9)
void initMotorInterrupts() {
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    for (int i = 0; i < 10; i++) {
        uint8_t exti_cr = i / 4;
        uint8_t exti_pos = (i % 4) * 4;

        SYSCFG->EXTICR[exti_cr] &= ~(0xF << exti_pos);
        SYSCFG->EXTICR[exti_cr] |= (0x4 << exti_pos); // PE0-PE9

        EXTI->FTSR |= (1UL << i); // Falling edge trigger
        EXTI->IMR |= (1UL << i);  // Enable interrupt
    }

    // Включаем прерывания в NVIC
    NVIC_EnableIRQ(EXTI0_IRQn);
    NVIC_EnableIRQ(EXTI1_IRQn);
    NVIC_EnableIRQ(EXTI2_IRQn);
    NVIC_EnableIRQ(EXTI3_IRQn);
    NVIC_EnableIRQ(EXTI4_IRQn);
    NVIC_EnableIRQ(EXTI9_5_IRQn);

    NVIC_SetPriority(EXTI0_IRQn, 2);
    NVIC_SetPriority(EXTI1_IRQn, 2);
    NVIC_SetPriority(EXTI2_IRQn, 2);
    NVIC_SetPriority(EXTI3_IRQn, 2);
    NVIC_SetPriority(EXTI4_IRQn, 2);
    NVIC_SetPriority(EXTI9_5_IRQn, 2);
}

// Инициализация прерываний для ENDSTOP (PE10-PE15)
void initEndstopInterrupts() {
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    for (int i = 10; i < 16; i++) {
        uint8_t exti_cr = i / 4;
        uint8_t exti_pos = (i % 4) * 4;

        SYSCFG->EXTICR[exti_cr] &= ~(0xF << exti_pos);
        SYSCFG->EXTICR[exti_cr] |= (0x4 << exti_pos); // PE10-PE15

        EXTI->FTSR |= (1UL << i); // Falling edge trigger
        EXTI->IMR |= (1UL << i);  // Enable interrupt
    }

    // Включаем прерывания в NVIC
    NVIC_EnableIRQ(EXTI15_10_IRQn);
    NVIC_SetPriority(EXTI15_10_IRQn, 0); // Высший приоритет для аварийной остановки
}
#endif // MOTOR_CODE_DISABLED

int main(void)
{
    SystemClock_HSI_Config();
    clear_usart4_rx_array();
    initGPIO();
    DMA_init();
    initSerial();
    SysTick_Init();

#if 0 // MOTOR_CODE_DISABLED
    // Задержка для инициализации. Нужно для того, чтобы
    // драйверы моторов успели инициализироваться
    for(uint32_t t = 0; t < 1000; ++t);
    GPIOC->ODR |= 0x03FF; // Вкл. подачу тока на все моторы по умолчанию
#endif // MOTOR_CODE_DISABLED

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    GPIOD->MODER |= GPIO_MODER_MODER12_0 | GPIO_MODER_MODER13_0 | GPIO_MODER_MODER14_0 | GPIO_MODER_MODER15_0;
    GPIOD->ODR |= GPIO_ODR_OD12 | GPIO_ODR_OD13 | GPIO_ODR_OD14 | GPIO_ODR_OD15;

#if 0 // MOTOR_CODE_DISABLED
    // Инициализируем прерывания
    initMotorInterrupts();
    initEndstopInterrupts();
#endif // MOTOR_CODE_DISABLED

    while (1)
    {
        if (g_packetReady)
        {
            g_packetReady = false;
            GPIOD->ODR ^= GPIO_ODR_OD12;

            processPacketCommand(g_packetParser);

            g_packetParser.reset();
            GPIOD->ODR ^= GPIO_ODR_OD13;
        }
    }
}

#if 0 // MOTOR_CODE_DISABLED
extern "C" void __attribute__((interrupt, used)) DMA1_Stream6_IRQHandler(void) // TX
{
    if ((DMA1->HISR & DMA_HISR_TCIF6) == DMA_HISR_TCIF6)
    {
        usart2_mrk = 0xFF;
        DMA1_Stream6->CR &= ~DMA_SxCR_EN;
        while ((DMA1_Stream6->CR & DMA_SxCR_EN) == DMA_SxCR_EN)
            ;
        DMA1->HIFCR |= DMA_HIFCR_CTCIF6;
    }
}
#endif // MOTOR_CODE_DISABLED

extern "C" void __attribute__((interrupt, used)) UART4_IRQHandler(void)
{
    if (UART4->SR & USART_SR_RXNE)
    {
        g_rxByte = static_cast<uint8_t>(UART4->DR);
        GPIOD->ODR ^= GPIO_ODR_OD14;

        if (g_packetParser.processByte(g_rxByte)) {
            g_packetReady = true;
        }
    }
}

#if 0 // MOTOR_CODE_DISABLED
// Обработчики прерываний для STATUS моторов (PE0-PE9)
extern "C" void __attribute__((interrupt, used)) EXTI0_IRQHandler(void)
{
    if (EXTI->PR & EXTI_PR_PR0)
    {
        completedMotors |= (1 << 0); // Мотор 1 завершил движение
        EXTI->PR = EXTI_PR_PR0;
    }
}

extern "C" void __attribute__((interrupt, used)) EXTI1_IRQHandler(void)
{
    if (EXTI->PR & EXTI_PR_PR1)
    {
        completedMotors |= (1 << 1); // Мотор 2 завершил движение
        EXTI->PR = EXTI_PR_PR1;
    }
}

extern "C" void __attribute__((interrupt, used)) EXTI2_IRQHandler(void)
{
    if (EXTI->PR & EXTI_PR_PR2)
    {
        completedMotors |= (1 << 2); // Мотор 3 завершил движение
        EXTI->PR = EXTI_PR_PR2;
    }
}

extern "C" void __attribute__((interrupt, used)) EXTI3_IRQHandler(void)
{
    if (EXTI->PR & EXTI_PR_PR3)
    {
        completedMotors |= (1 << 3); // Мотор 4 завершил движение
        EXTI->PR = EXTI_PR_PR3;
    }
}

extern "C" void __attribute__((interrupt, used)) EXTI4_IRQHandler(void)
{
    if (EXTI->PR & EXTI_PR_PR4)
    {
        completedMotors |= (1 << 4); // Мотор 5 завершил движение
        EXTI->PR = EXTI_PR_PR4;
    }
}

// Обработчик прерываний для STATUS моторов 6-10 (PE5-PE9)
extern "C" void __attribute__((interrupt, used)) EXTI9_5_IRQHandler(void)
{
    if (EXTI->PR & EXTI_PR_PR5) { completedMotors |= (1 << 5); EXTI->PR = EXTI_PR_PR5; }
    if (EXTI->PR & EXTI_PR_PR6) { completedMotors |= (1 << 6); EXTI->PR = EXTI_PR_PR6; }
    if (EXTI->PR & EXTI_PR_PR7) { completedMotors |= (1 << 7); EXTI->PR = EXTI_PR_PR7; }
    if (EXTI->PR & EXTI_PR_PR8) { completedMotors |= (1 << 8); EXTI->PR = EXTI_PR_PR8; }
    if (EXTI->PR & EXTI_PR_PR9) { completedMotors |= (1 << 9); EXTI->PR = EXTI_PR_PR9; }
}

// Обработчик прерываний для ENDSTOP (PE10-PE15) - аварийная остановка
extern "C" void __attribute__((interrupt, used)) EXTI15_10_IRQHandler(void)
{
    if (EXTI->PR & EXTI_PR_PR10) { emergencyStop = true; EXTI->PR = EXTI_PR_PR10; }
    if (EXTI->PR & EXTI_PR_PR11) { emergencyStop = true; EXTI->PR = EXTI_PR_PR11; }
    if (EXTI->PR & EXTI_PR_PR12) { emergencyStop = true; EXTI->PR = EXTI_PR_PR12; }
    if (EXTI->PR & EXTI_PR_PR13) { emergencyStop = true; EXTI->PR = EXTI_PR_PR13; }
    if (EXTI->PR & EXTI_PR_PR14) { emergencyStop = true; EXTI->PR = EXTI_PR_PR14; }
    if (EXTI->PR & EXTI_PR_PR15) { emergencyStop = true; EXTI->PR = EXTI_PR_PR15; }
}
#endif // MOTOR_CODE_DISABLED

extern "C" void __attribute__((interrupt, used)) SysTick_Handler(void)
{
    g_motorSimulator.tick();
}