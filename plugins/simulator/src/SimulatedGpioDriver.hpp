// plugins/gpio_simulator/src/SimulatedGpioDriver.hpp
#pragma once
#include "core/io/IIODriver.hpp"
#include "IGpioScenario.hpp"
#include <atomic>
#include <iostream>

class SimulatedGpioDriver final : public IIODriver {
public:
    // Указатель — не владеет, scenario живёт в GpioSimulatorPlugin
    void setScenario(IGpioScenario* s) noexcept { scenario_.store(s, std::memory_order_release); }

    const char* name() const override { return "GpioSimulator"; }

    Value read(const IOAddress& addr) override {
        auto* s = scenario_.load(std::memory_order_acquire);
        const auto size = decodeSize(addr.channel);
        if (!s) return Value::fromDInt(0);
        return s->onRead(addr.channel, size);
    }

    void write(const IOAddress& addr, const Value& value) override {
        auto* s = scenario_.load(std::memory_order_acquire);
        if (!s) return;
        const auto size = decodeSize(addr.channel);
        s->onWrite(addr.channel, size, value);
    }

private:
    // Извлечение размера из закодированного channel
    // (формат IOMapper::encodeChannel: [size:2 в битах 13..14])
    static GpioSize decodeSize(uint16_t channel) {
        return static_cast<GpioSize>((channel >> 13) & 0x03);
    }

    std::atomic<IGpioScenario*> scenario_{nullptr};
};