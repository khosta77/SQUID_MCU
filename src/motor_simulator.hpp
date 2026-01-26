#pragma once

#include <cstdint>
#include "constants.hpp"

class MotorSimulator {
public:
    MotorSimulator();

    void reset();
    void startMotor(uint8_t motorNum, uint32_t steps);
    void startMotors(const uint8_t* motorData, uint8_t motorCount);
    void tick();
    void stopAll();

    bool allComplete() const;
    bool isRunning() const;

    uint16_t getActiveMotors() const { return _activeMotors; }
    uint16_t getCompletedMotors() const { return _completedMotors; }

private:
    static constexpr uint32_t STEPS_PER_TICK = 1;

    volatile uint32_t _simulationTicks[MAX_MOTORS];
    volatile uint16_t _activeMotors;
    volatile uint16_t _completedMotors;
    volatile bool _running;
};

extern MotorSimulator g_motorSimulator;
