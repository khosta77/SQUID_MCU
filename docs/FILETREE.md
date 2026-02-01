# Структура файлов SQUID MCU

## Обзор проекта

```
SQUID_MCU/
├── src/                          # Исходный код прошивки
│   ├── main.cpp                  # Точка входа, инициализация, ISR
│   ├── motor_controller.cpp/hpp  # Обработка команд
│   ├── motor_driver.cpp/hpp      # FSM управления драйверами
│   ├── key_controller.cpp/hpp    # Управление KEY пинами (PB0-PB9)
│   ├── usart2_driver.cpp/hpp     # TX/RX через USART2
│   ├── motor_settings.cpp/hpp    # Класс MotorSettings
│   ├── protocol.cpp/hpp          # Парсер пакетов
│   ├── constants.cpp/hpp         # Константы протокола
│   ├── serial.cpp/hpp            # UART4 инициализация (PC)
│   ├── gpio.cpp/hpp              # GPIO инициализация
│   └── subdir.mk                 # Правила сборки
│
├── scripts/                      # Python CLI утилиты
│   ├── cli.py                    # Главный CLI (пакетный протокол)
│   ├── squid.py                  # Старый CLI (байтовый протокол)
│   └── squid/                    # Библиотека клиента
│       ├── __init__.py
│       ├── client.py             # SquidClient
│       ├── packet.py             # Packet, calculate_xor
│       ├── protocol.py           # Command, Response, ErrorCode
│       ├── motor.py              # MotorParams
│       ├── transport.py          # AsyncSerialTransport
│       └── errors.py             # SquidError, TimeoutError
│
├── tests/                        # Pytest тесты
│   ├── __init__.py
│   ├── conftest.py               # Фикстуры
│   ├── test_packet.py            # Unit: Packet, XOR
│   ├── test_motor.py             # Unit: MotorParams
│   └── test_integration.py       # Интеграционные тесты
│
├── docs/                         # Документация
│   ├── COMMAND.md                # Описание команд протокола PC-MCU
│   ├── DRIVER_PROTOCOL.md        # Протокол MCU-Driver
│   ├── MOTOR_DRIVER.md           # Описание MotorDriver FSM
│   ├── ARCHITECTURE.md           # Архитектура системы
│   └── FILETREE.md               # Этот файл
│
├── system/                       # CMSIS и системные файлы
│   ├── include/cmsis/            # CMSIS заголовки
│   └── src/                      # Системные исходники
│
├── ldscripts/                    # Скрипты линковщика
│   ├── mem.ld
│   ├── libs.ld
│   └── sections.ld
│
├── makefile                      # Основной Makefile
├── pyproject.toml                # Python зависимости (Poetry)
├── CLAUDE.md                     # Инструкции для Claude Code
├── ABOUT.md                      # Обзор системы
└── README.md                     # Главный README
```

## Файлы MCU

### main.cpp

| Функция | Описание |
|---------|----------|
| `main()` | Инициализация периферии, главный цикл |
| `SystemClock_HSI_Config()` | Настройка тактирования (HSI 16 MHz) |
| `SysTick_Init()` | Инициализация SysTick (1 мс) |
| `DMA_init()` | Настройка DMA для UART4 RX |
| `DMA1_Stream2_IRQHandler()` | Обработчик DMA (прием байтов) |
| `SysTick_Handler()` | Обработчик SysTick (симуляция моторов) |

### motor_controller.cpp

| Функция | Описание |
|---------|----------|
| `processPacketCommand()` | Диспетчер команд |
| `handleVersionCommand()` | Обработка VERSION |
| `handleStatusCommand()` | Обработка STATUS |
| `handleStopCommand()` | Обработка STOP |
| `handleSyncMoveCommand()` | Обработка SYNC_MOVE |
| `handleAsyncMoveCommand()` | Обработка ASYNC_MOVE |

### motor_driver.cpp

| Метод | Описание |
|-------|----------|
| `reset()` | Сброс FSM в состояние IDLE |
| `startMotors()` | Запуск движения моторов |
| `tick()` | Обновление FSM (из SysTick ISR, каждую 1 мс) |
| `stopAll()` | Немедленная остановка всех моторов |
| `allComplete()` | Проверка завершения всех моторов |
| `isRunning()` | Проверка активности FSM |
| `getActiveMotors()` | Битовая маска активных моторов |
| `getCompletedMotors()` | Битовая маска завершённых моторов |

### key_controller.cpp

| Метод | Описание |
|-------|----------|
| `setKey()` | Установить состояние KEY пина для мотора |
| `clearAll()` | Сбросить все KEY пины |
| `isKeySet()` | Проверить состояние KEY пина |

### usart2_driver.cpp

| Метод | Описание |
|-------|----------|
| `send()` | Отправить массив байт через USART2 |
| `sendByte()` | Отправить один байт |
| `hasData()` | Проверить наличие данных в RX буфере |
| `readByte()` | Прочитать байт из RX буфера |
| `waitTransmitComplete()` | Дождаться завершения передачи |

### protocol.cpp

| Метод | Описание |
|-------|----------|
| `PacketParser::processByte()` | Обработка входящего байта |
| `PacketParser::reset()` | Сброс парсера |
| `PacketParser::getCommand()` | Получение команды |
| `PacketParser::getData()` | Получение данных |

### serial.cpp

| Функция | Описание |
|---------|----------|
| `initSerial()` | Инициализация UART4 |
| `sendByte2PC()` | Отправка байта |
| `sendPacket()` | Отправка пакета |
| `sendErrorPacket()` | Отправка ошибки |
| `sendVersionResponse()` | Ответ VERSION |
| `sendStatusResponse()` | Ответ STATUS |
| `sendStopResponse()` | Ответ STOP |
| `sendMoveResponse()` | Ответ MOVE |

### constants.hpp

| Константа | Значение | Описание |
|-----------|----------|----------|
| `PROTOCOL_STX` | 0x02 | Стартовый байт |
| `PROTOCOL_MIN_PACKET_SIZE` | 5 | Минимальный размер пакета |
| `PROTOCOL_MAX_PACKET_SIZE` | 256 | Максимальный размер пакета |
| `MAX_MOTORS` | 10 | Максимум моторов |
| `FIRMWARE_VERSION` | 0x10 | Версия 1.0 |

## Файлы Python

### cli.py

| Команда | Описание |
|---------|----------|
| `version` | Получить версию прошивки |
| `status` | Получить состояние моторов |
| `stop` | Остановить все моторы |
| `move` | Запустить движение мотора |
| `multi-move` | Запустить несколько моторов |

### squid/client.py

| Метод | Описание |
|-------|----------|
| `get_version()` | Запрос версии |
| `get_status()` | Запрос состояния |
| `stop()` | Остановка |
| `sync_move()` | Синхронное движение |
| `async_move()` | Асинхронное движение |

## Что редактировать

### Добавление новой команды

1. **constants.hpp** - добавить код команды в `namespace Cmd`
2. **constants.hpp** - добавить код ответа в `namespace Response`
3. **motor_controller.cpp** - добавить case в `processPacketCommand()`
4. **motor_controller.cpp** - создать `handleXxxCommand()`
5. **serial.hpp/cpp** - добавить `sendXxxResponse()` если нужен особый формат

6. **scripts/squid/protocol.py** - добавить в `Command` и `Response`
7. **scripts/squid/client.py** - добавить метод
8. **scripts/cli.py** - добавить команду CLI
9. **tests/test_integration.py** - добавить тест

### Изменение формата пакета

1. **constants.hpp** - изменить константы протокола
2. **protocol.cpp** - изменить `PacketParser::processByte()`
3. **serial.cpp** - изменить `sendPacket()`
4. **scripts/squid/packet.py** - изменить `Packet.to_bytes()` и `Packet.from_bytes()`
5. **scripts/squid/protocol.py** - изменить константы
6. **tests/test_packet.py** - обновить тесты

### Добавление параметра мотора

1. **motor_settings.hpp/cpp** - добавить поле в `MotorSettings`
2. **scripts/squid/motor.py** - добавить поле в `MotorParams`
3. **tests/test_motor.py** - обновить тесты

## Тестирование

```bash
# Unit тесты (без MCU)
poetry run pytest tests/test_packet.py tests/test_motor.py -v

# Интеграционные тесты (с MCU)
poetry run pytest tests/test_integration.py -v --port /dev/tty.usbserial-*

# Все тесты
poetry run pytest -v
```

## Сборка

```bash
# Сборка прошивки
make clean && make all

# Прошивка через ST-Link
st-flash write main.bin 0x08000000

# Установка Python зависимостей
poetry install
```
