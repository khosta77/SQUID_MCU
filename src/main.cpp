#include "../system/include/cmsis/stm32f4xx.h"
#include "motor_controller.hpp"
#include "constants.hpp"
#include "motor_settings.hpp"

#include <string.h>

void GPIO_init()
{
    // Включаем тактирование всех GPIO портов
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN |
                     RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIODEN |
                     RCC_AHB1ENR_GPIOEEN;

    // GPIOA - USART интерфейсы
    // PA2 (USART2_TX) - альтернативная функция
    GPIOA->MODER &= ~GPIO_MODER_MODER2;
    GPIOA->MODER |= GPIO_MODER_MODER2_1;
    GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR2;
    GPIOA->AFR[0] |= (0x7 << 8); // AF7 для USART2_TX

    // PA3 (USART2_RX) - альтернативная функция
    GPIOA->MODER &= ~GPIO_MODER_MODER3;
    GPIOA->MODER |= GPIO_MODER_MODER3_1;
    GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR3;
    GPIOA->AFR[0] |= (0x7 << 12); // AF7 для USART2_RX

    // PA0 (USART4_TX) - альтернативная функция
    GPIOA->MODER &= ~GPIO_MODER_MODER0;
    GPIOA->MODER |= GPIO_MODER_MODER0_1;
    GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR0;
    GPIOA->AFR[0] |= (0x8 << 0); // AF8 для USART4_TX

    // PA1 (USART4_RX) - альтернативная функция
    GPIOA->MODER &= ~GPIO_MODER_MODER1;
    GPIOA->MODER |= GPIO_MODER_MODER1_1;
    GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR1;
    GPIOA->AFR[0] |= (0x8 << 4); // AF8 для USART4_RX

    // GPIOB - KEY1-10 (PB0-PB9) - входы с подтяжкой
    for (int i = 0; i < 10; i++) {
        GPIOB->MODER &= ~(3UL << (i * 2)); // Вход (00)
        GPIOB->PUPDR &= ~(3UL << (i * 2));
        GPIOB->PUPDR |= (1UL << (i * 2)); // Pull-up (01)
    }

    // GPIOC - EN1-10 (PC0-PC9) - выходы для включения драйверов
    for (int i = 0; i < 10; i++) {
        GPIOC->MODER &= ~(3UL << (i * 2));
        GPIOC->MODER |= (1UL << (i * 2)); // Выход (01)
        GPIOC->OTYPER &= ~(1UL << i); // Push-pull (0)
        GPIOC->OSPEEDR |= (3UL << (i * 2)); // Максимальная скорость (11)
        GPIOC->PUPDR &= ~(3UL << (i * 2)); // No pull (00)
        GPIOC->ODR &= ~(1UL << i); // Изначально выключены
    }

    // GPIOD - SELECT1-10 (PD0-PD9) - выходы для выбора драйверов
    for (int i = 0; i < 10; i++) {
        GPIOD->MODER &= ~(3UL << (i * 2));
        GPIOD->MODER |= (1UL << (i * 2)); // Выход (01)
        GPIOD->OTYPER &= ~(1UL << i); // Push-pull (0)
        GPIOD->OSPEEDR |= (3UL << (i * 2)); // Максимальная скорость (11)
        GPIOD->PUPDR &= ~(3UL << (i * 2)); // No pull (00)
        GPIOD->ODR &= ~(1UL << i); // Изначально не выбраны
    }

    // GPIOE - STATUS1-10 (PE0-PE9) - входы для статуса моторов
    for (int i = 0; i < 10; i++) {
        GPIOE->MODER &= ~(3UL << (i * 2)); // Вход (00)
        GPIOE->PUPDR &= ~(3UL << (i * 2));
        GPIOE->PUPDR |= (2UL << (i * 2)); // Pull-down (10)
    }

    // GPIOE - ENDSTOP1-6 (PE10-PE15) - входы для концевых выключателей
    for (int i = 10; i < 16; i++) {
        GPIOE->MODER &= ~(3UL << (i * 2)); // Вход (00)
        GPIOE->PUPDR &= ~(3UL << (i * 2));
        GPIOE->PUPDR |= (2UL << (i * 2)); // Pull-down (10)
    }
}

void USART_init()
{
    RCC->APB1ENR |= RCC_APB1ENR_UART4EN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    UART4->CR1 = 0; // Сбрасываем все биты
    UART4->CR2 = 0; // 1 стоп-бит
    UART4->CR3 = 0;

    // UART4 - связь с ПК
    uint32_t f_CK = SystemCoreClock; // Тактовая частота UART4 (16 МГц, HSI)
    uint32_t baud = 9600;
    uint32_t usartdiv = (f_CK + (baud / 2)) / (16 * baud); // Округление
    UART4->BRR = (usartdiv / 16) << 4 | (usartdiv % 16); 
    UART4->CR1 |= (USART_CR1_TE | USART_CR1_RE);
    UART4->CR3 |= USART_CR3_DMAR;  // Только DMA для приема
    UART4->CR1 |= USART_CR1_UE;

    // USART2 - связь с драйверами моторов
    USART2->BRR = 0x8B; // 115200
    USART2->CR1 |= (USART_CR1_TE | USART_CR1_RE);
    USART2->CR3 |= (USART_CR3_DMAT);
    USART2->CR1 |= USART_CR1_UE;
}

uint8_t usart4_mrk = 0x00;
uint8_t usart4_rx_array[256];

uint8_t usart2_mrk = 0xFF;

// Глобальная функция для отправки ответов через UART4
void sendResponse(uint8_t response) {
    while (!(UART4->SR & USART_SR_TXE));
    UART4->DR = response;
}

// Функции для работы с драйверами
void send2driver(const uint8_t *frame)
{
    while (usart2_mrk == 0x00)
        ;
    usart2_mrk = 0x00;
    DMA1_Stream6->M0AR = (uint32_t)frame;
    DMA1_Stream6->NDTR = 12;
    DMA1_Stream6->CR |= DMA_SxCR_EN;
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
        usart4_mrk = 0x0A;
        DMA1_Stream2->CR &= ~DMA_SxCR_EN;
        while ((DMA1_Stream2->CR & DMA_SxCR_EN) == DMA_SxCR_EN)
            ;
        DMA1->LIFCR |= DMA_LIFCR_CTCIF2;
    }
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
    DMA1_Stream2->NDTR = 256;
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
    GPIO_init();
    DMA_init();        // СНАЧАЛА DMA!
    USART_init();      // ПОТОМ USART!

    // Инициализируем индикаторный светодиод (PD14)
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    GPIOD->MODER |= GPIO_MODER_MODER14_0; // Устанавливаем как выход (01)
    GPIOD->ODR |= GPIO_ODR_OD14; // Включаем светодиод
    
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

            DMA1_Stream2->CR |= DMA_SxCR_EN;
        }
    }
}