// plugins/iec_runtime/include/IIecRuntime.hpp
#pragma once
#include <string>
#include <core/value.hpp>

class ISimulation {
public:
    virtual ~ISimulation() = default;
    virtual Value  read(uint16_t channel, uint8_t  size) = 0;
    virtual void   write(uint16_t channel, uint8_t  size, const Value& val) = 0;
    virtual void run() = 0;
};
