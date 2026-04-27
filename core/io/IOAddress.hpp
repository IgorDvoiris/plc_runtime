#pragma once
// Физический адрес IO-точки
struct IOAddress {
    uint8_t  bus;      // шина / интерфейс (0 = GPIO, 1 = ModBus, ...)
    uint16_t device;   // адрес устройства на шине
    uint16_t channel;  // канал / регистр внутри устройства

    bool operator==(const IOAddress& o) const {
        return bus == o.bus && device == o.device && channel == o.channel;
    }
};
