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

        GpioDriver gpio(
            // read: симуляция чтения GPIO
            [](uint16_t pin) -> bool {
                (void)pin;
                std::cout << "GPIO>[" << pin << "] = " << false << "\n";
                return false; // TODO: реальное чтение
            },
            // write: симуляция записи GPIO
            [](uint16_t pin, bool val) {
                std::cout << "<GPIO[" << pin << "] = " << val << "\n";
            }
        );

        iomapper_->registerDriver("gpio", &gpio);
        // ── Автоматическая привязка AT-переменных ─────────────────
        // Для INPUT (%I) и OUTPUT (%Q) возвращаем gpioDriver,
        // для MEMORY (%M) — nullptr (внутренняя переменная, IO не нужен)
        iomapper_->autoBindDirectAddresses(
            [&](DirectAddressPrefix pfx, uint8_t /*bus*/) -> IIODriver* {
                switch (pfx) {
                    case DirectAddressPrefix::INPUT:
                    case DirectAddressPrefix::OUTPUT:
                        return &gpio;
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