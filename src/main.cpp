#include "../system/include/cmsis/stm32f4xx.h"
#include "motor_controller.hpp"
#include "constants.hpp"
#include "motor_settings.hpp"
#include "gpio.hpp"
#include "serial.hpp"

#include <string.h>

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

void theTimeoutWorked()
{
    waitingForMotorData = false;
    sendByte2PC(0x0D);
}

// Функция ожидания данных моторов и их обработки
void waitForMotorDataAndProcess(uint8_t command, uint8_t motorCount) {
    uint32_t timeoutLimit = 5000000; // 5 секунд при 16 МГц
    GPIOD->ODR &= ~GPIO_ODR_OD13;

#if 0
    while (waitingForMotorData && !timeoutOccurred) {
        // Проверяем таймаут
        if (timeoutCounter++ > timeoutLimit) {
            timeoutOccurred = true;
            break;
        }
        
        // Проверяем аварийную остановку
        if (emergencyStop) {
            emergencyStopAllMotors();
            sendByte2PC(0x0B);
            waitingForMotorData = false;
            return;
        }
        
        // Короткая задержка для предотвращения перегрузки CPU
        for (volatile uint32_t i = 0; i < 1000; i++);
    }
#endif

    stopDMAStream2();

    GPIOD->ODR |= GPIO_ODR_OD13;  // Оранжевый индикатор
    
    if (timeoutOccurred)
    {
        return theTimeoutWorked();
    }

    GPIOD->ODR &= ~GPIO_ODR_OD15;
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

void DMA_init()
{
    // DMA1_Stream6 this USART2_TX
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
    DMA1_Stream6->CR &= ~DMA_SxCR_EN;
    while ((DMA1_Stream6->CR & DMA_SxCR_EN) == DMA_SxCR_EN)
        ;

    // - (0x4 << 25) - 4-ый канал
    // - DMA_SxCR_MINC - увеличенный объем памяти
    // - DMA_SxCR_TCIE - прерывания по приему/передачи
    // - DMA_SxCR_CIRC (for rx) - циклическая работа
    DMA1_Stream6->CR |= ((0x4 << 25) | DMA_SxCR_MINC | DMA_SxCR_TCIE);
    DMA1_Stream6->CR &= ~(DMA_SxCR_MSIZE | DMA_SxCR_PSIZE);
    DMA1_Stream6->CR |= (0x01 << 6); // Из памяти в перефирию
    DMA1_Stream6->NDTR = 12;
    DMA1_Stream6->PAR = (uint32_t)(&USART2->DR);
    // DMA1_Stream6->M0AR = (uint32_t) &usart2_tx_array[0];
    NVIC_EnableIRQ(DMA1_Stream6_IRQn);
    NVIC_SetPriority(DMA1_Stream6_IRQn, 7);

    // DMA1_Stream2 this UART4_RX
    DMA1_Stream2->CR &= ~DMA_SxCR_EN;
    while ((DMA1_Stream2->CR & DMA_SxCR_EN) == DMA_SxCR_EN)
        ;

    // - (0x4 << 25) - 4-ый канал
    // - DMA_SxCR_MINC - увеличенный объем памяти
    // - DMA_SxCR_TCIE - прерывания по приему/передачи
    // - DMA_SxCR_CIRC (for rx) - циклическая работа
    DMA1_Stream2->CR |= ((0x4 << 25) | DMA_SxCR_MINC | DMA_SxCR_TCIE | DMA_SxCR_CIRC);
    DMA1_Stream2->CR &= ~(DMA_SxCR_MSIZE | DMA_SxCR_PSIZE);
    DMA1_Stream2->CR &= ~(3UL << 6); // Из переферии в память
    DMA1_Stream2->NDTR = 1;
    DMA1_Stream2->PAR = (uint32_t)(&UART4->DR);
    DMA1_Stream2->M0AR = (uint32_t)&usart4_rx_array[0];
    NVIC_EnableIRQ(DMA1_Stream2_IRQn);
    NVIC_SetPriority(DMA1_Stream2_IRQn, 5);

    DMA1_Stream2->CR |= DMA_SxCR_EN;
    while ((DMA1_Stream2->CR & DMA_SxCR_EN) != DMA_SxCR_EN)
        ;
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

int main(void)
{
    SystemClock_HSI_Config();
    clear_usart4_rx_array();
    initGPIO();
    DMA_init();
    initSerial();

    // Задержка для инициализации. Нужно для того, чтобы
    // драйверы моторов успели инициализироваться
    for(uint32_t t = 0; t < 1000; ++t);
    GPIOC->ODR |= 0x03FF; // Вкл. подачу тока на все моторы по умолчанию

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    GPIOD->MODER |= GPIO_MODER_MODER12_0 | GPIO_MODER_MODER13_0 | GPIO_MODER_MODER14_0 | GPIO_MODER_MODER15_0; // Устанавливаем как выход (01)
    GPIOD->ODR |= GPIO_ODR_OD12 | GPIO_ODR_OD13 | GPIO_ODR_OD14 | GPIO_ODR_OD15; // Включаем светодиод
    
    // Инициализируем прерывания
    initMotorInterrupts();
    initEndstopInterrupts();

    while (1)
    {
        if (usart4_mrk)
        {
            usart4_mrk = 0x00;
            while ((DMA1_Stream2->CR & DMA_SxCR_EN) == DMA_SxCR_EN)
                ;

            if (usart4_rx_array[0] != 0) {
                processCommand(usart4_rx_array[0]);
            }

            stopDMAStream2();
            clear_usart4_rx_array();
            GPIOD->ODR |= GPIO_ODR_OD12 | GPIO_ODR_OD13 | GPIO_ODR_OD14 | GPIO_ODR_OD15; // Включаем светодиод
            DMA1_Stream2->NDTR = 1;
            DMA1_Stream2->CR |= DMA_SxCR_EN;
        }
    }
}

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

extern "C" void __attribute__((interrupt, used)) DMA1_Stream2_IRQHandler(void) // UART4 RX
{
    if ((DMA1->LISR & DMA_LISR_TCIF2) == DMA_LISR_TCIF2)
    {
        DMA1_Stream2->CR &= ~DMA_SxCR_EN;
        while ((DMA1_Stream2->CR & DMA_SxCR_EN) == DMA_SxCR_EN)
            ;
        DMA1->LIFCR |= DMA_LIFCR_CTCIF2;
        GPIOD->ODR ^= GPIO_ODR_OD14; 
        if (waitingForMotorData) {
            timeoutCounter = 0;
            usart4_mrk = 0x0A; // Сигнализируем о получении данных
        } else {
            usart4_mrk = 0x0A;
        }
    }
}

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