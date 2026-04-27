// IIODriver.hpp
#include <core/io/IOAddress.hpp>
#pragma once
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