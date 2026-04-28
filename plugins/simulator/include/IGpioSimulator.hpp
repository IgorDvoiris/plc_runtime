// plugins/gpio_simulator/include/IGpioSimulator.hpp
#pragma once
#include "IGpioScenario.hpp"
#include <memory>

class IGpioSimulator {
public:
    virtual ~IGpioSimulator() = default;

    // Установить активный сценарий. Симулятор берёт ownership.
    // Если scenario==nullptr — fallback на passive mode (читаем 0, write игнорируем).
    virtual void setScenario(std::unique_ptr<IGpioScenario> scenario) = 0;

    // Текущий сценарий (read-only)
    virtual IGpioScenario* scenario() const = 0;
};