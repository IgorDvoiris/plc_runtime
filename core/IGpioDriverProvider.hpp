// core/IGpioDriverProvider.hpp
#pragma once

class IIODriver;  // forward (определён в code/io/IIODriver.hpp)

// Сервис, регистрируемый в SystemBus.
// Любой плагин (симулятор или реальный hardware-driver) может зарегистрировать
// поставщика GPIO-драйвера. VM Runtime получает driver через этот интерфейс.
class IGpioDriverProvider {
public:
    virtual ~IGpioDriverProvider() = default;
    virtual IIODriver* gpioDriver() = 0;
};