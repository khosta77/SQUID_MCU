# MotorDriver - управление драйверами моторов

## Обзор

MotorDriver — модуль управления шаговыми драйверами через USART2. Реализует конечный автомат (FSM) для последовательной отправки команд на драйверы моторов.

## Файлы

| Файл | Назначение |
|------|------------|
| `src/motor_driver.hpp` | Объявление класса MotorDriver, FSM состояния |
| `src/motor_driver.cpp` | Реализация логики отправки команд |
| `src/key_controller.hpp` | Объявление KeyController (управление PB0-PB9) |
| `src/key_controller.cpp` | Реализация управления KEY пинами |
| `src/usart2_driver.hpp` | Объявление Usart2Driver (TX/RX через USART2) |
| `src/usart2_driver.cpp` | Реализация низкоуровневой работы с USART2 |

## State Machine

```
         startMotors()
              │
              ▼
┌──────────────────────────┐
│          IDLE            │◄─────────────────────────────┐
└────────────┬─────────────┘                              │
             │ есть pending моторы                        │
             ▼                                            │
┌──────────────────────────┐                              │
│      CHECKING_RX         │  Проверка: нет ли данных    │
└────────────┬─────────────┘  в RX буфере                │
             │ RX чист                                    │
             ▼                                            │
┌──────────────────────────┐                              │
│        SENDING           │  KEY HIGH + TX packet       │
└────────────┬─────────────┘                              │
             │ TX complete                                │
             ▼                                            │
┌──────────────────────────┐                              │
│    WAITING_TIMEOUT       │  Ожидание ответа / таймаут  │
└────────────┬─────────────┘  (3 сек в debug режиме)     │
             │ таймаут истёк или ответ получен           │
             ▼                                            │
┌──────────────────────────┐                              │
│        COMPLETE          │  KEY LOW                    │
└────────────┬─────────────┘                              │
             │                                            │
             ├── есть ещё pending ────► CHECKING_RX      │
             │                                            │
             └── все отправлены ──────────────────────────┘
```

## API

### Основные методы

| Метод | Описание |
|-------|----------|
| `startMotors(data, count)` | Запуск движения моторов из бинарных данных |
| `tick()` | Обновление FSM, вызывается из SysTick каждую 1 мс |
| `stopAll()` | Немедленная остановка всех моторов |
| `reset()` | Сброс состояния в IDLE |

### Методы проверки состояния

| Метод | Описание |
|-------|----------|
| `allComplete()` | Возвращает true если все моторы завершили работу |
| `isRunning()` | Возвращает true если FSM активен |
| `getActiveMotors()` | Битовая маска активных моторов |
| `getCompletedMotors()` | Битовая маска завершённых моторов |
| `getState()` | Текущее состояние FSM |

## Вспомогательные классы

### KeyController

Управление KEY пинами (PB0-PB9) для выбора активного драйвера.

```cpp
KeyController::setKey(motorNum, true);   // Активировать драйвер
KeyController::setKey(motorNum, false);  // Деактивировать драйвер
KeyController::clearAll();               // Деактивировать все
KeyController::isKeySet(motorNum);       // Проверить состояние
```

### Usart2Driver

Низкоуровневая работа с USART2.

```cpp
Usart2Driver::send(data, length);        // Отправить массив байт
Usart2Driver::sendByte(data);            // Отправить один байт
Usart2Driver::hasData();                 // Есть ли данные в RX
Usart2Driver::readByte();                // Прочитать байт из RX
Usart2Driver::waitTransmitComplete();    // Дождаться завершения TX
```

## Debug Mode

В текущей реализации используется debug-режим:
- Вместо ожидания реального ответа от драйвера используется таймаут 3 секунды
- Константа: `DEBUG_TIMEOUT_MS = 3000`
- После истечения таймаута мотор считается "завершившим" работу

## Интеграция с main.cpp

```cpp
// SysTick_Handler вызывает tick() каждую 1 мс
extern "C" void SysTick_Handler(void) {
    g_motorDriver.tick();
}

// В обработчике команд SYNC_MOVE
g_motorDriver.startMotors(data, count);
while (!g_motorDriver.allComplete()) {
    __WFI();  // Ждём завершения
}
```

## Формат входных данных

Метод `startMotors()` принимает массив структур MotorSettings (16 байт на мотор):

```
┌────────────┬──────────────┬───────────┬─────────┐
│   number   │ acceleration │ max_speed │  steps  │
│  uint32_t  │   uint32_t   │  uint32_t │ int32_t │
│  4 bytes   │   4 bytes    │  4 bytes  │ 4 bytes │
└────────────┴──────────────┴───────────┴─────────┘
```
