#include "motor_controller.hpp"
#include "../system/include/cmsis/stm32f4xx.h"
#include <string.h>

#if 0 // MOTOR_CODE_DISABLED
// Forward declarations
void launchAsyncMotor(uint8_t motorNumber);
void stopAllMotors();
void processSyncMode(uint8_t motorCount);
void processAsyncMode(uint8_t motorCount);
void emergencyStopAllMotors();
bool checkAllMotorsCompleted();
void resetMotorStates();

// Глобальные переменные для отслеживания состояния моторов определены в constants.cpp

void configureMotor(uint8_t motorNumber, const MotorSettings& params) {
    GPIOD->ODR |= (1UL << (motorNumber - 1)); // Включаем SELECT
    GPIOB->ODR |= (1UL << (motorNumber - 1)); // Включаем KEY
    for(uint32_t t = 0; t < 100; ++t);

    // Отправляем параметры драйверу (12 байт: ускорение, скорость, шаги)
    send2driver(reinterpret_cast<const uint8_t*>(&params));

    // Ожидаем завершения отправки данных по DMA
    while (usart2_mrk == 0x00);

    GPIOB->ODR &= ~(1UL << (motorNumber - 1)); // Выключаем KEY
    for(uint32_t t = 0; t < 100; ++t);
}

void launchAsyncMotor(uint8_t motorNumber) {
    GPIOD->ODR &= ~(1UL << (motorNumber - 1)); // Выключаем SELECT после настройки
}

void stopAllMotors() {
    // АВАРИЙНАЯ ОСТАНОВКА: Выключаем все моторы при срабатывании концевого выключателя
    // Выключаем все EN (PC0-PC9) - отключаем подачу тока
    GPIOC->ODR &= 0xFC00;

    // Выключаем все SELECT (PD0-PD9) - снимаем выбор моторов
    GPIOD->ODR &= 0xFC00;

    // Выключаем все KEY (PB0-PB9) - отключаем передачу данных
    GPIOB->ODR &= 0xFC00;
}
#endif // MOTOR_CODE_DISABLED

void processCommand(uint8_t command) {
#if 0 // MOTOR_CODE_DISABLED
    // Проверяем аварийную остановку
    if (emergencyStop) {
        emergencyStopAllMotors();
        sendByte2PC(0x0B); // Код ошибки аварийной остановки
        return;
    }

    // Сбрасываем состояние моторов
    resetMotorStates();
#endif // MOTOR_CODE_DISABLED

    // Обработка команды запроса версии
    if (IS_VERSION_COMMAND(command)) {
        sendByte2PC(FIRMWARE_VERSION);
        return;
    }

#if 0 // MOTOR_CODE_DISABLED
    // Получаем количество моторов
    uint8_t motorCount = GET_MOTOR_COUNT(command);

    // Валидация количества моторов
    if (!VALIDATE_MOTOR_COUNT(motorCount)) {
        sendByte2PC(0x01); // Ошибка: некорректное количество моторов
        return;
    }

    // НОВАЯ ЛОГИКА: Настраиваем ожидание данных моторов
    expectedDataSize = motorCount * 16; // 16 байт на мотор
    waitingForMotorData = true;
    timeoutCounter = 0;
    timeoutOccurred = false;

    // Настраиваем DMA на ожидание данных моторов
    clear_usart4_rx_array();
    DMA1_Stream2->NDTR = expectedDataSize;
    DMA1_Stream2->M0AR = (uint32_t)&usart4_rx_array[0];
    DMA1_Stream2->CR |= DMA_SxCR_EN;
    while ((DMA1_Stream2->CR & DMA_SxCR_EN) != DMA_SxCR_EN)
        ;
    // Отправляем подтверждение готовности
    sendByte2PC(RESPONSE_READY);

    // НЕ ВЫХОДИМ из функции! Ждем данные или таймаут
    waitForMotorDataAndProcess(command, motorCount);
#else
    sendByte2PC(0x01); // Неизвестная команда в тестовом режиме
#endif // MOTOR_CODE_DISABLED
}

#if 0 // MOTOR_CODE_DISABLED
void processSyncMode(uint8_t motorCount) {
    syncMotorBuffer = 0; // Сбрасываем буфер синхронного режима

    // Устанавливаем количество активных моторов
    currentMotorCount = motorCount;
    activeMotors = (1 << motorCount) - 1; // Устанавливаем биты для моторов 1-motorCount

    for (uint8_t i = 0; i < motorCount; i++) {

        MotorSettings params(i, &usart4_rx_array[0]);

        if (!params) {
            sendByte2PC(static_cast<uint8_t>(params.getNumber()));
            sendByte2PC(0xAA); // Ошибка валидации параметров
            return;
        }

        configureMotor(params.getNumber(), params);
        syncMotorBuffer |= (1UL << (params.getNumber() - 1));
    }

    // Запускаем все моторы одновременно
    GPIOD->ODR &= ~syncMotorBuffer;

#if 0 // TODO раскрытить когда будет время.
    // Ждем завершения всех моторов или аварийной остановки
    while (!checkAllMotorsCompleted() && !emergencyStop) {
        // Проверяем аварийную остановку
        if (emergencyStop) {
            emergencyStopAllMotors();
            sendByte2PC(0x0B); // Код ошибки аварийной остановки
            return;
        }
    }
#endif

    if (!emergencyStop) {
        sendByte2PC(RESPONSE_SUCCESS);
    }
}

void processAsyncMode(uint8_t motorCount) {
    // Устанавливаем количество активных моторов
    currentMotorCount = motorCount;
    activeMotors = (1 << motorCount) - 1; // Устанавливаем биты для моторов 1-motorCount

    // Асинхронный режим - настраиваем и запускаем по мере готовности
    for (uint8_t i = 0; i < motorCount; i++) {
        // Парсим параметры мотора
        MotorSettings params(i, usart4_rx_array);

        // Валидируем параметры
        if (!params) {
            sendByte2PC(0x01); // Ошибка валидации параметров
            return;
        }

        configureMotor(params.getNumber(), params);
        launchAsyncMotor(params.getNumber());
    }

#if 0
    // Ждем завершения всех моторов или аварийной остановки
    while (!checkAllMotorsCompleted() && !emergencyStop) {
        // Проверяем аварийную остановку
        if (emergencyStop) {
            emergencyStopAllMotors();
            sendByte2PC(0x0B); // Код ошибки аварийной остановки
            return;
        }
    }
#endif

    if (!emergencyStop) {
        sendByte2PC(RESPONSE_SUCCESS);
    }
}

// Функции состояния системы
void emergencyStopAllMotors() {
    stopAllMotors();

    // Сбрасываем состояние
    activeMotors = 0;
    completedMotors = 0;
    emergencyStop = false;
    currentMotorCount = 0;
    syncMotorBuffer = 0;
}

bool checkAllMotorsCompleted() {
    return (activeMotors & completedMotors) == activeMotors;
}

void resetMotorStates() {
    activeMotors = 0;
    completedMotors = 0;
    emergencyStop = false;
    currentMotorCount = 0;
    syncMotorBuffer = 0;
}
#endif // MOTOR_CODE_DISABLED
