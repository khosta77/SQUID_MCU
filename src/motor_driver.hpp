#pragma once

#include <cstdint>
#include "constants.hpp"
#include "motor_settings.hpp"

enum class DriverState : uint8_t {
    IDLE,
    CHECKING_RX,
    SENDING,
    WAITING_TIMEOUT,
    COMPLETE
};

class MotorDriver {
public:
    MotorDriver();

    void reset();
    void startMotors(const uint8_t* motorData, uint8_t motorCount);
    void tick();
    void stopAll();

    bool allComplete() const;
    bool isRunning() const;

    uint16_t getActiveMotors() const { return _activeMotors; }
    uint16_t getCompletedMotors() const { return _completedMotors; }

    DriverState getState() const { return _state; }

private:
    static constexpr uint8_t TX_BUFFER_SIZE = 14;
    static constexpr uint32_t DEBUG_TIMEOUT_MS = 3000;

    MotorSettings _settings[MAX_MOTORS];
    uint8_t _txBuffer[TX_BUFFER_SIZE];

    volatile DriverState _state;
    volatile uint16_t _activeMotors;
    volatile uint16_t _completedMotors;
    volatile uint16_t _pendingMotors;
    volatile uint8_t _motorCount;
    volatile uint8_t _currentSendIndex;
    volatile uint32_t _timeoutCounter;
    volatile bool _running;

    uint8_t buildDriverPacket(const MotorSettings& settings);
    void sendCommandToDriver(uint8_t motorNum);
    void processNextMotor();
    void startSending();
};

extern MotorDriver g_motorDriver;
