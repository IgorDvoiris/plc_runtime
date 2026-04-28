// plugins/gpio_simulator/include/IGpioScenario.hpp
#pragma once
#include "core/io/value.hpp"
#include <cstdint>

enum class GpioSize : uint8_t { BIT, BYTE, WORD, DWORD };

class IGpioScenario {
public:
    virtual ~IGpioScenario() = default;

    // Вызывается симулятором при read() от VM.
    // channel — закодированный uint16_t (формат IOMapper::encodeChannel).
    virtual Value onRead(uint16_t channel, GpioSize size) = 0;

    // Вызывается симулятором при write() от VM.
    virtual void  onWrite(uint16_t channel, GpioSize size, const Value& v) = 0;

    // Опциональный tick для time-based сценариев.
    // Вызывается симулятором, если scenario зарегистрирован в task'е.
    virtual void  tick(uint64_t /*nowMs*/) {}

    // Метаданные (для логирования)
    virtual const char* name() const = 0;
};