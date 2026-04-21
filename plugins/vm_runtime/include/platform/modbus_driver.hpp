// modbus_driver.hpp
#pragma once
#include "io_driver.hpp"
#include <vector>
#include <cstdint>
#include <functional>

// Modbus RTU/TCP драйвер
// channel кодирует тип регистра и адрес:
//   channel = (type << 12) | register_address
//   type: 0=Coil, 1=DiscreteInput, 2=InputReg, 3=HoldingReg
class ModbusDriver : public IIODriver {
public:
    // Функция транзакции: read/write одного регистра
    using TransactFn = std::function<
        uint16_t(uint8_t devAddr, uint8_t fnCode, uint16_t regAddr,
                 uint16_t writeVal, bool isWrite)>;

    explicit ModbusDriver(TransactFn fn) : transact_(std::move(fn)) {}

    const char* name() const override { return "Modbus"; }

    Value read(const IOAddress& addr) override {
        uint8_t  regType = static_cast<uint8_t>(addr.channel >> 12);
        uint16_t regAddr = addr.channel & 0x0FFF;
        uint8_t  fnCode  = readFnCode(regType);

        uint16_t raw = transact_(
            static_cast<uint8_t>(addr.device),
            fnCode, regAddr, 0, false);

        // Coil / DiscreteInput → BOOL
        if (regType == 0 || regType == 1)
            return Value::fromBool(raw != 0);

        // InputReg / HoldingReg → INT
        return Value::fromInt(static_cast<int16_t>(raw));
    }

    void write(const IOAddress& addr, const Value& value) override {
        uint8_t  regType = static_cast<uint8_t>(addr.channel >> 12);
        uint16_t regAddr = addr.channel & 0x0FFF;
        uint8_t  fnCode  = writeFnCode(regType);
        uint16_t raw     = static_cast<uint16_t>(value.asInt());

        transact_(static_cast<uint8_t>(addr.device),
                  fnCode, regAddr, raw, true);
    }

    // Batch: один Modbus-запрос для группы регистров
    void beginBatch() override { batchRegs_.clear(); }
    void endBatch()   override { /* отправить накопленный запрос */ }

private:
    TransactFn           transact_;
    std::vector<uint16_t> batchRegs_;

    static uint8_t readFnCode(uint8_t regType) {
        switch (regType) {
            case 0:  return 0x01; // Read Coils
            case 1:  return 0x02; // Read Discrete Inputs
            case 2:  return 0x04; // Read Input Registers
            default: return 0x03; // Read Holding Registers
        }
    }

    static uint8_t writeFnCode(uint8_t regType) {
        return (regType == 0) ? 0x05 : 0x06; // Write Single Coil/Register
    }
};