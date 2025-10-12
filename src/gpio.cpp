#include "gpio.hpp"
#include "../system/include/cmsis/stm32f4xx.h"

void initGPIO()
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
