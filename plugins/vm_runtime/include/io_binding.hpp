// io_binding.hpp
#pragma once
#include "value.hpp"
#include "../platform/io_driver.hpp"
#include <string>
#include <vector>
#include <cstdint>

// Одна привязка: физический адрес ↔ переменная VM
struct IOBinding {
    // Физическая сторона
    IOAddress   ioAddr;
    IIODriver*  driver      = nullptr;   // не владеет объектом

    // Сторона VM
    std::string pouName;     // имя POU
    std::string varName;     // имя переменной
    uint16_t    varIndex;    // кэшированный индекс в фрейме

    // Метаданные
    IODirection direction;
    ValueType   valueType;   // ожидаемый тип
    std::string description; // для отладки/логирования

    // Масштабирование для аналоговых сигналов (опционально)
    bool  scaleEnabled = false;
    float rawMin  = 0.0f,  rawMax  = 1.0f;
    float engMin  = 0.0f,  engMax  = 1.0f;

    // Применить масштабирование raw → engineering units
    Value applyScale(const Value& raw) const {
        if (!scaleEnabled) return raw;
        float r = raw.asReal();
        float ratio = (r - rawMin) / (rawMax - rawMin);
        return Value::fromReal(engMin + ratio * (engMax - engMin));
    }

    // Применить обратное масштабирование EU → raw
    Value applyScaleInverse(const Value& eng) const {
        if (!scaleEnabled) return eng;
        float e = eng.asReal();
        float ratio = (e - engMin) / (engMax - engMin);
        return Value::fromReal(rawMin + ratio * (rawMax - rawMin));
    }
};

// Построитель привязки (fluent interface)
class IOBindingBuilder {
public:
    explicit IOBindingBuilder(IOBinding& b) : b_(b) {}

    IOBindingBuilder& bus    (uint8_t  v) { b_.ioAddr.bus     = v; return *this; }
    IOBindingBuilder& device (uint16_t v) { b_.ioAddr.device  = v; return *this; }
    IOBindingBuilder& channel(uint16_t v) { b_.ioAddr.channel = v; return *this; }
    IOBindingBuilder& driver (IIODriver* d){ b_.driver        = d; return *this; }
    IOBindingBuilder& pou    (std::string n){ b_.pouName = std::move(n); return *this; }
    IOBindingBuilder& var    (std::string n){ b_.varName = std::move(n); return *this; }
    IOBindingBuilder& dir    (IODirection d){ b_.direction     = d; return *this; }
    IOBindingBuilder& type   (ValueType   t){ b_.valueType     = t; return *this; }
    IOBindingBuilder& desc   (std::string s){ b_.description = std::move(s); return *this; }

    IOBindingBuilder& scale(float rawMin, float rawMax,
                             float engMin, float engMax) {
        b_.scaleEnabled = true;
        b_.rawMin = rawMin; b_.rawMax = rawMax;
        b_.engMin = engMin; b_.engMax = engMax;
        return *this;
    }

private:
    IOBinding& b_;
};