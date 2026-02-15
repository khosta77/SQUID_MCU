#include "motor_driver.hpp"
#include "key_controller.hpp"
#include "usart2_driver.hpp"
#include "../system/include/cmsis/stm32f4xx.h"
#include <cstring>

MotorDriver g_motorDriver;

MotorDriver::MotorDriver() {
    reset();
}

void MotorDriver::reset() {
    _state = DriverState::IDLE;
    _activeMotors = 0;
    _completedMotors = 0;
    _pendingMotors = 0;
    _motorCount = 0;
    _currentSendIndex = 0;
    _timeoutCounter = 0;
    _running = false;
    for (uint8_t i = 0; i < MAX_MOTORS; ++i) {
        _debounceCounters[i] = 0;
    }
    KeyController::clearAll();
}

void MotorDriver::startMotors(const uint8_t* motorData, uint8_t motorCount) {
    reset();

    _motorCount = motorCount;

    for (uint8_t i = 0; i < motorCount; ++i) {
        _settings[i] = MotorSettings(i, motorData);
        uint8_t motorNum = static_cast<uint8_t>(_settings[i].getNumber());
        if (motorNum >= 1 && motorNum <= MAX_MOTORS) {
            _activeMotors |= (1U << (motorNum - 1));
        }
    }

    _running = true;
    _state = DriverState::CHECKING_RX;
}

uint8_t MotorDriver::buildDriverPacket(const MotorSettings& settings) {
    _txBuffer[0] = DRIVER_CMD;

    uint32_t accel = settings.getAcceleration();
    std::memcpy(&_txBuffer[1], &accel, 4);

    uint32_t speed = settings.getMaxSpeed();
    std::memcpy(&_txBuffer[5], &speed, 4);

    int32_t steps = static_cast<int32_t>(settings.getSteps());
    std::memcpy(&_txBuffer[9], &steps, 4);

    uint8_t xorVal = 0;
    for (uint8_t i = 0; i < 13; ++i) {
        xorVal ^= _txBuffer[i];
    }
    _txBuffer[13] = xorVal;

    return TX_BUFFER_SIZE;
}

void MotorDriver::sendCommandToDriver(uint8_t motorNum) {
    KeyController::setKey(motorNum, true);
    for (volatile uint32_t i = 0; i < 1000; ++i);

    uint8_t len = buildDriverPacket(_settings[_currentSendIndex]);
    Usart2Driver::send(_txBuffer, len);
    Usart2Driver::waitTransmitComplete();

    for (volatile uint32_t i = 0; i < 1000; ++i);
    KeyController::setKey(motorNum, false);

    _pendingMotors |= (1U << (motorNum - 1));
}

void MotorDriver::processNextMotor() {
    if (_currentSendIndex < _motorCount) {
        uint8_t motorNum = static_cast<uint8_t>(_settings[_currentSendIndex].getNumber());
        if (motorNum >= 1 && motorNum <= MAX_MOTORS) {
            sendCommandToDriver(motorNum);
        }
        _currentSendIndex++;
    }

    if (_currentSendIndex >= _motorCount) {
        _state = DriverState::WAITING_STATUS;
        _timeoutCounter = 0;
    }
}

void MotorDriver::startSending() {
    _currentSendIndex = 0;
    _state = DriverState::SENDING;
}

void MotorDriver::tick() {
    if (!_running) {
        return;
    }

    switch (_state) {
        case DriverState::IDLE:
            break;

        case DriverState::CHECKING_RX: {
            GPIOD->ODR |= GPIO_ODR_OD13;
            uint8_t maxReads = 32;
            while (Usart2Driver::hasData() && maxReads > 0) {
                Usart2Driver::readByte();
                maxReads--;
            }
            startSending();
            break;
        }

        case DriverState::SENDING:
            GPIOD->ODR |= GPIO_ODR_OD14;
            processNextMotor();
            break;

        case DriverState::WAITING_STATUS: {
            _timeoutCounter++;
            uint16_t statusBits = GPIOE->IDR & 0x03FF;
            for (uint8_t i = 0; i < MAX_MOTORS; ++i) {
                if (_pendingMotors & (1U << i)) {
                    if (!(statusBits & (1U << i))) {
                        _debounceCounters[i]++;
                        if (_debounceCounters[i] >= DEBOUNCE_MS) {
                            _completedMotors |= (1U << i);
                            _pendingMotors &= ~(1U << i);
                        }
                    } else {
                        _debounceCounters[i] = 0;
                    }
                }
            }
            if (_pendingMotors == 0) {
                _state = DriverState::COMPLETE;
                GPIOD->ODR |= GPIO_ODR_OD15;
            } else if (_timeoutCounter >= SAFETY_TIMEOUT_MS) {
                _completedMotors = _activeMotors;
                _pendingMotors = 0;
                _state = DriverState::COMPLETE;
                GPIOD->ODR |= GPIO_ODR_OD15;
            }
            break;
        }

        case DriverState::COMPLETE:
            _running = false;
            GPIOD->ODR &= ~(GPIO_ODR_OD13 | GPIO_ODR_OD14);
            break;
    }
}

void MotorDriver::stopAll() {
    _running = false;
    _completedMotors = _activeMotors;
    _pendingMotors = 0;
    _state = DriverState::IDLE;
    KeyController::clearAll();
}

bool MotorDriver::allComplete() const {
    return (_activeMotors != 0) && ((_activeMotors & _completedMotors) == _activeMotors);
}

bool MotorDriver::isRunning() const {
    return _running;
}
