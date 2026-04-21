// plugins/iec_runtime/src/IecProgram.cpp
#include "io_mapper.hpp"
#include "gpio_driver.hpp"
#include "VmProgram.hpp"
#include <iostream>
#include "vm.hpp"
#include "serializer.hpp"
#include <iostream>

VmProgram::VmProgram(const std::string& instanceName, VmRawBuffer& rawBuf)
    : instanceName_(instanceName), rawBuf_(rawBuf) {}

bool VmProgram::load(const std::string& soPath) {
    std::cout << "[VmProgram] " << instanceName_ << "] loading " << soPath << "\n";
    try {
        vm_ = std::make_unique<VirtualMachine>();

        auto bcFile = Serializer::load(soPath);
 
        for (auto& pou : bcFile.pous) {
            vm_->loadPOU(pou);
        }

        iomapper_ = std::make_unique<IOMapper>(*vm_);

        GpioDriver::getInstance().configure(
            // ── READ ──────────────────────────────────────────────
            [](uint16_t channel, DirectAddressSize size) -> Value {
                auto da = IOMapper::decodeChannel(channel);
                std::cout << "rGPIO>[" << channel << " (" << da.toString() << ", size="
                        << static_cast<int>(size) << ")]";

                switch (size) {
                    case DirectAddressSize::BIT: {
                        // TODO: to be removed. Simulation.
                        bool val = (da.byteAddr == 0 && da.bitAddr == 0);
                        std::cout << " = " << val << "\n";
                        return Value::fromBool(val);
                    }
                    case DirectAddressSize::BYTE:
                        return Value::fromInt(0); // Reserved for SINT/USINT

                    case DirectAddressSize::WORD: {
                        // TODO: to be removed. Simulation.
                        if (da.byteAddr == 1) {
                            std::cout << " = 85 (simulated INT)\n";
                            return Value::fromInt(85);
                        }
                        return Value::fromInt(0);
                    }
                    case DirectAddressSize::DWORD:
                        // %ID4 и т.д.
                        return Value::fromDInt(0);

                    default:
                        return Value::fromDInt(0);
                }
            },

            // ── WRITE ─────────────────────────────────────────────
            [](uint16_t channel, DirectAddressSize size, const Value& val) {
                auto da = IOMapper::decodeChannel(channel);
                std::cout << "wGPIO>[" << channel << " (" << da.toString() << ", size="
                        << static_cast<int>(size) << ")]";

                switch (size) {
                    case DirectAddressSize::BIT:
                        std::cout << " = " << val.asBool() << "\n";
                        break;
                    case DirectAddressSize::WORD:
                        std::cout << " = " << static_cast<int16_t>(val.asInt()) << "\n";
                        break;
                    case DirectAddressSize::DWORD:
                        std::cout << " = " << val.asInt() << "\n";
                        break;
                    default:
                        std::cout << " (unsupported type)\n";
                }
            }
        );

        iomapper_->registerDriver("gpio", &GpioDriver::getInstance());
        // ── Автоматическая привязка AT-переменных ─────────────────
        // Для INPUT (%I) и OUTPUT (%Q) возвращаем gpioDriver,
        // для MEMORY (%M) — nullptr (внутренняя переменная, IO не нужен)
        iomapper_->autoBindDirectAddresses(
            [&](DirectAddressPrefix pfx, uint8_t /*bus*/) -> IIODriver* {
                switch (pfx) {
                    case DirectAddressPrefix::INPUT:
                    case DirectAddressPrefix::OUTPUT:
                        return &GpioDriver::getInstance();
                    case DirectAddressPrefix::MEMORY:
                        return nullptr; // %M — только внутренняя память
                }
                return nullptr;
            }
        );
        iomapper_->initialize();

        std::cout << "[VmProgram] VM PLC Runtime loaded. Press Ctrl+C to stop.\n";
        return true;
    } catch (const std::exception& ex) {
        std::cerr << "[VmProgram] VM PLC Runtime fatal: " << ex.what() << "\n";
        return false;
    }
}

void VmProgram::tick() {
    iomapper_->readInputs();
    vm_->executeCycle(instanceName_);
    iomapper_->writeOutputs();
}

VmProgram::~VmProgram() = default;