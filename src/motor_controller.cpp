#include "motor_controller.hpp"
#include "../system/include/cmsis/stm32f4xx.h"
#include <string.h>

// Forward declarations для статических функций
static void selectMotorDriver(uint8_t motorNumber);
static void deselectMotorDriver(uint8_t motorNumber);
static void enableMotorDriver(uint8_t motorNumber);
static void disableMotorDriver(uint8_t motorNumber);
static void launchMotor(uint8_t motorNumber);
static void launchMotorsSync(uint16_t motorMask);
static void stopAllMotors();
static uint8_t getMotorCount(uint8_t command);
static bool isSyncCommand(uint8_t command);
static bool isAsyncCommand(uint8_t command);
static bool isVersionCommand(uint8_t command);
static bool validateMotorCount(uint8_t count);
static void processSyncMode(uint8_t motorCount);
static void processAsyncMode(uint8_t motorCount);
static void emergencyStopAllMotors();
static bool checkAllMotorsCompleted();
static void resetMotorStates();

// Глобальные переменные для отслеживания состояния моторов
volatile uint16_t activeMotors = 0;        // Битовое поле активных моторов
volatile uint16_t completedMotors = 0;     // Битовое поле завершенных моторов
volatile bool emergencyStop = false;       // Флаг аварийной остановки
volatile uint8_t currentMotorCount = 0;    // Количество моторов в текущей команде
volatile uint16_t syncMotorBuffer = 0;     // Битовое поле моторов для синхронного запуска

// Внешние переменные из main.cpp
extern uint8_t usart4_rx_array[256];
extern uint8_t usart2_mrk;

// Функции управления моторами
static void selectMotorDriver(uint8_t motorNumber) {
    if (motorNumber >= 1 && motorNumber <= MAX_MOTORS) {
        GPIOD->ODR |= (1UL << (motorNumber - 1)); // Включаем SELECT
    }
}

static void deselectMotorDriver(uint8_t motorNumber) {
    if (motorNumber >= 1 && motorNumber <= MAX_MOTORS) {
        GPIOD->ODR &= ~(1UL << (motorNumber - 1)); // Выключаем SELECT
    }
}

static void enableMotorDriver(uint8_t motorNumber) {
    if (motorNumber >= 1 && motorNumber <= MAX_MOTORS) {
        GPIOC->ODR |= (1UL << (motorNumber - 1)); // Включаем EN
    }
}

static void disableMotorDriver(uint8_t motorNumber) {
    if (motorNumber >= 1 && motorNumber <= MAX_MOTORS) {
        GPIOC->ODR &= ~(1UL << (motorNumber - 1)); // Выключаем EN
    }
}

void configureMotor(uint8_t motorNumber, const MotorSettings& params) {
    if (motorNumber < 1 || motorNumber > MAX_MOTORS) return;
    
    // Выбираем драйвер мотора
    selectMotorDriver(motorNumber);
    
    // Включаем драйвер для настройки
    enableMotorDriver(motorNumber);
    
    // Отправляем параметры драйверу (12 байт)
    send2driver(reinterpret_cast<const uint8_t*>(&params));
    
    // Ждем готовности мотора (проверяем STATUS)
    while (!(GPIOE->IDR & (1UL << (motorNumber - 1)))) {
        // Ждем готовности
    }
    
    // Выключаем драйвер после настройки
    disableMotorDriver(motorNumber);
    
    // Снимаем выбор драйвера
    deselectMotorDriver(motorNumber);
}

static void launchMotor(uint8_t motorNumber) {
    if (motorNumber < 1 || motorNumber > MAX_MOTORS) return;
    
    // Включаем драйвер для запуска
    enableMotorDriver(motorNumber);
}

static void launchMotorsSync(uint16_t motorMask) {
    // Одновременно запускаем все моторы из маски
    GPIOC->ODR |= motorMask;
}

static void stopAllMotors() {
    // Выключаем все EN (PC0-PC9)
    GPIOC->ODR &= 0xFC00;
    
    // Выключаем все SELECT (PD0-PD9)
    GPIOD->ODR &= 0xFC00;
}

// Функции обработки команд протокола

static uint8_t getMotorCount(uint8_t command) {
    return command & CMD_MOTOR_COUNT_MASK;
}

static bool isSyncCommand(uint8_t command) {
    return (command & CMD_SYNC_MASK) == CMD_SYNC_MASK;
}

static bool isAsyncCommand(uint8_t command) {
    return (command & CMD_ASYNC_MASK) == CMD_ASYNC_MASK;
}

static bool isVersionCommand(uint8_t command) {
    return (command & CMD_VERSION_MASK) == CMD_VERSION_MASK;
}

static bool validateMotorCount(uint8_t count) {
    return count >= 1 && count <= MAX_MOTORS;
}

// Функции обработки команд
void processCommand(uint8_t command) {
    // Проверяем аварийную остановку
    if (emergencyStop) {
        emergencyStopAllMotors();
        sendResponse(0x0B); // Код ошибки аварийной остановки
        return;
    }

    // Сбрасываем состояние моторов
    resetMotorStates();

    // Обработка команды запроса версии
    if (isVersionCommand(command)) {
        sendResponse(FIRMWARE_VERSION);
        return;
    }

    // Получаем количество моторов
    uint8_t motorCount = getMotorCount(command);
    
    // Валидация количества моторов
    if (!validateMotorCount(motorCount)) {
        sendResponse(0x01); // Ошибка: некорректное количество моторов
        return;
    }

    // Устанавливаем количество активных моторов
    currentMotorCount = motorCount;
    activeMotors = (1 << motorCount) - 1; // Устанавливаем биты для моторов 1-motorCount

    // Отправляем подтверждение готовности
    sendResponse(RESPONSE_READY);

    // Определяем режим работы
    if (isSyncCommand(command)) {
        // Синхронный режим - настраиваем все моторы, затем запускаем одновременно
        processSyncMode(motorCount);
    } else if (isAsyncCommand(command)) {
        // Асинхронный режим - настраиваем и запускаем по мере готовности
        processAsyncMode(motorCount);
    } else {
        sendResponse(0x02); // Ошибка: некорректный режим работы
    }
}

static void processSyncMode(uint8_t motorCount) {
    syncMotorBuffer = 0; // Сбрасываем буфер синхронного режима
    
    // Настраиваем все моторы по очереди
    for (uint8_t i = 0; i < motorCount; i++) {
        // Парсим параметры мотора
        MotorSettings params(i, usart4_rx_array);
        
        // Валидируем параметры
        if (!params) {
            sendResponse(0x01); // Ошибка валидации параметров
            return;
        }
        
        // Настраиваем мотор
        configureMotor(params.getNumber(), params);
        
        // Добавляем мотор в буфер синхронного запуска
        syncMotorBuffer |= (1UL << (params.getNumber() - 1));
    }
    
    // Запускаем все моторы одновременно
    launchMotorsSync(syncMotorBuffer);
    
    // Ждем завершения всех моторов или аварийной остановки
    while (!checkAllMotorsCompleted() && !emergencyStop) {
        // Проверяем аварийную остановку
        if (emergencyStop) {
            emergencyStopAllMotors();
            sendResponse(0x0B); // Код ошибки аварийной остановки
            return;
        }
    }
    
    if (!emergencyStop) {
        sendResponse(RESPONSE_SUCCESS);
    }
}

static void processAsyncMode(uint8_t motorCount) {
    // Асинхронный режим - настраиваем и запускаем по мере готовности
    for (uint8_t i = 0; i < motorCount; i++) {
        // Парсим параметры мотора
        MotorSettings params(i, usart4_rx_array);
        
        // Валидируем параметры
        if (!params) {
            sendResponse(0x01); // Ошибка валидации параметров
            return;
        }
        
        // Настраиваем мотор
        configureMotor(params.getNumber(), params);
        
        // Сразу запускаем мотор (асинхронный режим)
        launchMotor(params.getNumber());
    }
    
    // Ждем завершения всех моторов или аварийной остановки
    while (!checkAllMotorsCompleted() && !emergencyStop) {
        // Проверяем аварийную остановку
        if (emergencyStop) {
            emergencyStopAllMotors();
            sendResponse(0x0B); // Код ошибки аварийной остановки
            return;
        }
    }
    
    if (!emergencyStop) {
        sendResponse(RESPONSE_SUCCESS);
    }
}

// Функции состояния системы
static void emergencyStopAllMotors() {
    stopAllMotors();
    
    // Сбрасываем состояние
    activeMotors = 0;
    completedMotors = 0;
    emergencyStop = false;
    currentMotorCount = 0;
    syncMotorBuffer = 0;
}

static bool checkAllMotorsCompleted() {
    return (activeMotors & completedMotors) == activeMotors;
}

static void resetMotorStates() {
    activeMotors = 0;
    completedMotors = 0;
    emergencyStop = false;
    currentMotorCount = 0;
    syncMotorBuffer = 0;
}
