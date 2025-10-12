#pragma once

#include <cstdint>

class MotorSettings {
public:
    MotorSettings() = default;
    
    MotorSettings(uint32_t number, uint32_t acceleration, uint32_t maxSpeed, uint32_t steps)
        : number_(number), acceleration_(acceleration), maxSpeed_(maxSpeed), steps_(steps) {}
    
    /**
     * @brief Конструктор для парсинга параметров мотора из данных
     * @param motorIndex Индекс мотора в массиве данных
     * @param rxData Указатель на массив данных для парсинга
     */
    MotorSettings(uint8_t motorIndex, const uint8_t* rxData);
    MotorSettings(const MotorSettings& other) = default;
    MotorSettings& operator=(const MotorSettings& other) = default;
    ~MotorSettings() = default;
    
    /**
     * @brief Получить номер мотора
     * @return Номер мотора
     */
    uint32_t getNumber() const;
    
    /**
     * @brief Получить значение ускорения
     * @return Значение ускорения
     */
    uint32_t getAcceleration() const;
    
    /**
     * @brief Получить максимальную скорость
     * @return Максимальная скорость
     */
    uint32_t getMaxSpeed() const;
    
    /**
     * @brief Получить количество шагов
     * @return Количество шагов
     */
    uint32_t getSteps() const;
    
    /**
     * @brief Установить номер мотора
     * @param number Номер мотора
     */
    void setNumber(uint32_t number);
    
    /**
     * @brief Установить значение ускорения
     * @param acceleration Значение ускорения
     */
    void setAcceleration(uint32_t acceleration);
    
    /**
     * @brief Установить максимальную скорость
     * @param maxSpeed Максимальная скорость
     */
    void setMaxSpeed(uint32_t maxSpeed);
    
    /**
     * @brief Установить количество шагов
     * @param steps Количество шагов
     */
    void setSteps(uint32_t steps);
    
    /**
     * @brief Оператор сравнения на равенство
     * @param other Другой объект MotorSettings для сравнения
     * @return true если объекты равны, false в противном случае
     */
    bool operator==(const MotorSettings& other) const;
    
    /**
     * @brief Оператор сравнения на неравенство
     * @param other Другой объект MotorSettings для сравнения
     * @return true если объекты не равны, false в противном случае
     */
    bool operator!=(const MotorSettings& other) const;
    
    /**
     * @brief Оператор преобразования в bool для использования в условиях
     * @return true если параметры валидны, false в противном случае
     */
    explicit operator bool() const;

private:
    uint32_t acceleration_ = 0;  // Ускорение
    uint32_t maxSpeed_ = 0;      // Максимальная скорость
    uint32_t steps_ = 0;         // Количество шагов
    uint32_t number_ = 0;        // Номер мотора

    /**
     * @brief Проверить валидность параметров мотора
     * @return true если все параметры валидны, false в противном случае
     */
    bool isValid() const;
};
