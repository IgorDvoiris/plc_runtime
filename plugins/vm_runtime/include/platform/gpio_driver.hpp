// gpio_driver.hpp
#pragma once
#include "io_driver.hpp"
#include <functional>
#include <unordered_map>

#if 0
// Абстрактный GPIO-драйвер с колбэками
// (реальная реализация зависит от платформы)
class GpioDriver : public IIODriver {
public:
    using ReadFn  = std::function<bool(uint16_t pin)>;
    using WriteFn = std::function<void(uint16_t pin, bool value)>;

    GpioDriver(ReadFn reader, WriteFn writer)
        : reader_(std::move(reader))
        , writer_(std::move(writer))
    {}

    const char* name() const override { return "GPIO"; }

    Value read(const IOAddress& addr) override {
        return Value::fromBool(reader_(addr.channel));
    }

    void write(const IOAddress& addr, const Value& value) override {
        writer_(addr.channel, value.asBool());
    }

private:
    ReadFn  reader_;
    WriteFn writer_;
};

#else

#include <mutex>
#include <functional>

class GpioDriver : public IIODriver {
public:
    using ReadFn  = std::function<bool(uint16_t pin)>;
    using WriteFn = std::function<void(uint16_t pin, bool value)>;

    // Удаляем возможность копирования
    GpioDriver(const GpioDriver&) = delete;
    GpioDriver& operator=(const GpioDriver&) = delete;

    static GpioDriver& getInstance() {
        // Гарантированно один экземпляр на всю программу
        static GpioDriver instance;
        return instance;
    }

    // Метод для настройки логики (вызывается один раз при старте)
    void configure(ReadFn reader, WriteFn writer) {
        std::lock_guard<std::mutex> lock(mutex_);
        reader_ = std::move(reader);
        writer_ = std::move(writer);
    }

    const char* name() const override { return "GPIO"; }

    Value read(const IOAddress& addr) override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (reader_) {
            return Value::fromBool(reader_(addr.channel));
        }
        return Value::fromBool(false);
    }

    void write(const IOAddress& addr, const Value& value) override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (writer_) {
            writer_(addr.channel, value.asBool());
        }
    }

private:
    // Приватный конструктор
    GpioDriver() = default;

    std::mutex mutex_;
    ReadFn  reader_;
    WriteFn writer_;
};

#endif