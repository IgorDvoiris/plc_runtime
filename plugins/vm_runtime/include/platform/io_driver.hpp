// io_driver.hpp
#pragma once
#include "value.hpp"
#include <string>
#include <cstdint>

// Направление привязки
enum class IODirection : uint8_t {
    INPUT  = 0,   // физика → VM (читаем перед циклом)
    OUTPUT = 1,   // VM → физика (пишем после цикла)
    INOUT  = 2,   // двунаправленный
};

// Физический адрес IO-точки
struct IOAddress {
    uint8_t  bus;      // шина / интерфейс (0 = GPIO, 1 = ModBus, ...)
    uint16_t device;   // адрес устройства на шине
    uint16_t channel;  // канал / регистр внутри устройства

    bool operator==(const IOAddress& o) const {
        return bus == o.bus && device == o.device && channel == o.channel;
    }
};

// Хэш для использования в unordered_map
struct IOAddressHash {
    size_t operator()(const IOAddress& a) const {
        size_t h = a.bus;
        h = h * 31 + a.device;
        h = h * 31 + a.channel;
        return h;
    }
};

// Интерфейс драйвера конкретного типа IO
class IIODriver {
public:
    virtual ~IIODriver() = default;

    // Имя драйвера для регистрации и отладки
    virtual const char* name() const = 0;

    // Инициализация/деинициализация
    virtual bool init()     { return true; }
    virtual void shutdown() {}

    // Чтение одного канала
    virtual Value read (const IOAddress& addr) = 0;

    // Запись одного канала
    virtual void  write(const IOAddress& addr, const Value& value) = 0;

    // Пакетное обновление (опционально, для оптимизации)
    // Вызывается один раз перед/после цикла для группы каналов одного драйвера
    virtual void beginBatch() {}
    virtual void endBatch()   {}
};