#include "motor_settings.hpp"
#include "constants.hpp"


MotorSettings::MotorSettings(uint8_t motorIndex, const uint8_t* rxData) {
    uint8_t offset = (motorIndex * 16);
    
    // Данные приходят как: 0x01, 0x00, 0x00, 0x00 -> должно быть 0x00000001
    // Это означает, что данные в little-endian формате, но перевернуты
    // Попробуем обратный порядок байтов
    number_ = (rxData[offset + 3] << 24) | (rxData[offset + 2] << 16) | 
              (rxData[offset + 1] << 8) | rxData[offset];
    
    acceleration_ = (rxData[offset + 7] << 24) | (rxData[offset + 6] << 16) | 
                    (rxData[offset + 5] << 8) | rxData[offset + 4];
    
    maxSpeed_ = (rxData[offset + 11] << 24) | (rxData[offset + 10] << 16) | 
                (rxData[offset + 9] << 8) | rxData[offset + 8];
    
    steps_ = (rxData[offset + 15] << 24) | (rxData[offset + 14] << 16) | 
             (rxData[offset + 13] << 8) | rxData[offset + 12];
    
}

uint32_t MotorSettings::getNumber() const {
    return number_;
}

uint32_t MotorSettings::getAcceleration() const {
    return acceleration_;
}

uint32_t MotorSettings::getMaxSpeed() const {
    return maxSpeed_;
}

uint32_t MotorSettings::getSteps() const {
    return steps_;
}

void MotorSettings::setNumber(uint32_t number) {
    number_ = number;
}

void MotorSettings::setAcceleration(uint32_t acceleration) {
    acceleration_ = acceleration;
}

void MotorSettings::setMaxSpeed(uint32_t maxSpeed) {
    maxSpeed_ = maxSpeed;
}

void MotorSettings::setSteps(uint32_t steps) {
    steps_ = steps;
}

bool MotorSettings::operator==(const MotorSettings& other) const {
    return number_ == other.number_ &&
           acceleration_ == other.acceleration_ &&
           maxSpeed_ == other.maxSpeed_ &&
           steps_ == other.steps_;
}

bool MotorSettings::operator!=(const MotorSettings& other) const {
    return !(*this == other);
}

MotorSettings::operator bool() const {
    return isValid();
}

bool MotorSettings::isValid() const {
    if (number_ < 1 || number_ > MAX_MOTORS) {
        return false;
    }
    
    if (acceleration_ == 0) {
        return false;
    }
    
    if (maxSpeed_ == 0) {
        return false;
    }
    
    return true;
}