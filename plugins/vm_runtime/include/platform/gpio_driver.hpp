// gpio_driver.hpp
#pragma once
#include "io_driver.hpp"
#include <core/io/IIODriver.hpp>
#include <core/io/IOAddress.hpp>
#include <functional>
#include <mutex>

class GpioDriver : public IIODriver {
public:
    using ReadFn  = std::function<Value(uint16_t channel, DirectAddressSize size)>;
    using WriteFn = std::function<void(uint16_t channel, DirectAddressSize size, const Value& val)>;

    GpioDriver(const GpioDriver&) = delete;
    GpioDriver& operator=(const GpioDriver&) = delete;

    static GpioDriver& getInstance() {
        static GpioDriver instance;
        return instance;
    }

    void configure(ReadFn reader, WriteFn writer) {
        std::lock_guard<std::mutex> lock(mutex_);
        reader_ = std::move(reader);
        writer_ = std::move(writer);
    }

    const char* name() const override { return "GPIO"; }

    Value read(const IOAddress& addr) override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!reader_) return Value::fromDInt(0);
        auto da = IOMapper::decodeChannel(addr.channel);
        return reader_(addr.channel, da.size);
    }

    void write(const IOAddress& addr, const Value& value) override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!writer_) return;
        auto da = IOMapper::decodeChannel(addr.channel);
        writer_(addr.channel, da.size, value);
    }

private:
    GpioDriver() = default;
    std::mutex mutex_;
    ReadFn  reader_;
    WriteFn writer_;
};