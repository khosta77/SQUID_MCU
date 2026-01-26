#include "motor_simulator.hpp"

MotorSimulator g_motorSimulator;

MotorSimulator::MotorSimulator() {
    reset();
}

void MotorSimulator::reset() {
    for (uint8_t i = 0; i < MAX_MOTORS; ++i) {
        _simulationTicks[i] = 0;
    }
    _activeMotors = 0;
    _completedMotors = 0;
    _running = false;
}

void MotorSimulator::startMotor(uint8_t motorNum, uint32_t steps) {
    if (motorNum < 1 || motorNum > MAX_MOTORS) {
        return;
    }

    uint8_t idx = motorNum - 1;
    _simulationTicks[idx] = (steps + STEPS_PER_TICK - 1) / STEPS_PER_TICK;
    if (_simulationTicks[idx] == 0) {
        _simulationTicks[idx] = 1;
    }

    _activeMotors |= (1U << idx);
    _running = true;
}

void MotorSimulator::startMotors(const uint8_t* motorData, uint8_t motorCount) {
    reset();

    for (uint8_t i = 0; i < motorCount; ++i) {
        uint32_t motorNum = *reinterpret_cast<const uint32_t*>(motorData + i * 16);
        uint32_t steps = *reinterpret_cast<const uint32_t*>(motorData + i * 16 + 12);
        startMotor(static_cast<uint8_t>(motorNum), steps);
    }
}

void MotorSimulator::tick() {
    if (!_running) {
        return;
    }

    for (uint8_t i = 0; i < MAX_MOTORS; ++i) {
        if ((_activeMotors & (1U << i)) && !(_completedMotors & (1U << i))) {
            if (_simulationTicks[i] > 0) {
                _simulationTicks[i]--;
            }

            if (_simulationTicks[i] == 0) {
                _completedMotors |= (1U << i);
            }
        }
    }

    if (allComplete()) {
        _running = false;
    }
}

void MotorSimulator::stopAll() {
    _running = false;
    _completedMotors = _activeMotors;
}

bool MotorSimulator::allComplete() const {
    return (_activeMotors != 0) && ((_activeMotors & _completedMotors) == _activeMotors);
}

bool MotorSimulator::isRunning() const {
    return _running;
}
