#include "key_controller.hpp"
#include "constants.hpp"
#include "../system/include/cmsis/stm32f4xx.h"

void KeyController::setKey(uint8_t motorNum, bool state) {
    if (motorNum < 1 || motorNum > MAX_MOTORS) {
        return;
    }
    uint8_t pin = motorNum - 1;
    if (state) {
        GPIOB->ODR |= (1UL << pin);
    } else {
        GPIOB->ODR &= ~(1UL << pin);
    }
}

void KeyController::clearAll() {
    GPIOB->ODR &= ~0x3FF;
}

bool KeyController::isKeySet(uint8_t motorNum) {
    if (motorNum < 1 || motorNum > MAX_MOTORS) {
        return false;
    }
    uint8_t pin = motorNum - 1;
    return (GPIOB->ODR & (1UL << pin)) != 0;
}
