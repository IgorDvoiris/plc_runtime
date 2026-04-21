// io_mapper.hpp
#pragma once
#include "io_binding.hpp"
#include "vm.hpp"
#include "io_driver.hpp"
#include "direct_address_parser.hpp"
#include <vector>
#include <unordered_map>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <functional>
#include <iostream>

// Колбэк для обработки ошибок IO (не бросает исключений в цикле)
using IOErrorHandler = std::function<void(
    const IOBinding&,
    const std::string& message)>;

class IOMapper {
public:
    explicit IOMapper(VirtualMachine& vm) : vm_(vm) {}

    // ── Регистрация драйверов ─────────────────────────────────
    void registerDriver(const std::string& name,
                        IIODriver* driver)
    {
        if (!driver)
            throw std::invalid_argument("[iomapper] Driver is null: " + name);
        drivers_[name] = driver;
    }

    IIODriver* findDriver(const std::string& name) const {
        auto it = drivers_.find(name);
        return (it != drivers_.end()) ? it->second : nullptr;
    }

    // ── Добавление привязок ───────────────────────────────────

    // Fluent-style добавление
    IOBindingBuilder addBinding() {
        bindings_.emplace_back();
        return IOBindingBuilder(bindings_.back());
    }

    // Прямое добавление готовой привязки
    void addBinding(IOBinding binding) {
        bindings_.push_back(std::move(binding));
    }

    // ── Инициализация (вызывается один раз перед запуском) ────
    void initialize() {
        // Инициализируем все драйверы
        if (drivers_.empty()) {
            std::cout << "[iomapper] Drivers are empty" << "\n";
            return;
        }
        for (auto& kv : drivers_) {
            if (!kv.second->init())
                throw std::runtime_error(
                    "[iomapper] IO driver init failed: " + kv.first);
        }

        // Кэшируем индексы переменных
        resolveVariableIndices();
    }

    void shutdown() {
        for (auto& kv : drivers_)
            kv.second->shutdown();
    }

    // ── Установка обработчика ошибок ──────────────────────────
    void setErrorHandler(IOErrorHandler handler) {
        errorHandler_ = std::move(handler);
    }

    // ── Основные методы цикла сканирования ───────────────────

    // Вызывается ДО executeCycle(): физика → VM
    void readInputs() {
        // Группируем вызовы по драйверу для batch-оптимизации
        IIODriver* lastDriver = nullptr;

        for (auto& binding : bindings_) {
            if (binding.direction != IODirection::INPUT &&
                binding.direction != IODirection::INOUT)
                continue;
            if (!binding.driver) continue;

            // Batch boundary
            if (binding.driver != lastDriver) {
                if (lastDriver) lastDriver->endBatch();
                lastDriver = binding.driver;
                lastDriver->beginBatch();
            }

            try {
                Value raw  = binding.driver->read(binding.ioAddr);
                Value eng  = binding.applyScale(raw);
                Value conv = convertType(eng, binding.valueType);
                writeToVM(binding, conv);
            } catch (const std::exception& ex) {
                handleError(binding, ex.what());
            }
        }

        if (lastDriver) lastDriver->endBatch();
    }

    // Вызывается ПОСЛЕ executeCycle(): VM → физика
    void writeOutputs() {
        IIODriver* lastDriver = nullptr;

        for (auto& binding : bindings_) {
            if (binding.direction != IODirection::OUTPUT &&
                binding.direction != IODirection::INOUT)
                continue;
            if (!binding.driver) continue;

            if (binding.driver != lastDriver) {
                if (lastDriver) lastDriver->endBatch();
                lastDriver = binding.driver;
                lastDriver->beginBatch();
            }

            try {
                Value eng  = readFromVM(binding);
                Value raw  = binding.applyScaleInverse(eng);
                binding.driver->write(binding.ioAddr, raw);
            } catch (const std::exception& ex) {
                handleError(binding, ex.what());
            }
        }

        if (lastDriver) lastDriver->endBatch();
    }

    // Доступ к таблице привязок (для отладки)
    const std::vector<IOBinding>& bindings() const { return bindings_; }

    // Автоматически создать привязки для всех AT-переменных всех POU
    // driverForPrefix — колбэк: возвращает драйвер по префиксу и шине
    void autoBindDirectAddresses(
        std::function<IIODriver*(DirectAddressPrefix, uint8_t bus)> driverForPrefix)
    {
        for (const POUBytecode* pou : vm_.getPOUs()) {
            if (!pou) continue;

            for (const VarDecl& v : pou->vars) {
                if (!v.hasDirectAddress) continue;

                const DirectAddress& da = v.directAddress;

                IIODriver* drv = driverForPrefix(da.prefix, 0);
                if (!drv) continue; // %M или нет драйвера → пропускаем

                // Проверяем: не создана ли уже привязка для этой переменной
                bool alreadyBound = false;
                for (auto& existing : bindings_) {
                    if (existing.pouName  == pou->name &&
                        existing.varIndex == v.index) {
                        alreadyBound = true;
                        break;
                    }
                }
                if (alreadyBound) continue;

                IOBinding binding;
                binding.pouName     = pou->name;
                binding.varName     = v.name;
                binding.varIndex    = v.index;
                binding.driver      = drv;
                binding.direction   = DirectAddressParser::toIODirection(da.prefix);
                binding.valueType   = DirectAddressParser::toValueType(da.size);
                binding.description = v.name + " @ " + da.toString();
                binding.ioAddr.bus     = 0;
                binding.ioAddr.device  = 0;
                binding.ioAddr.channel = encodeChannel(da);

                std::cout << "[iomapper] iomapper_->autoBindDirectAddresses "  << " name: " << binding.varName << " index: "
                    <<  binding.varIndex << " type: " << static_cast<int>(binding.valueType) << "\n";

                bindings_.push_back(std::move(binding));
            }
        }
    }

    // Кодирование DirectAddress → uint16_t channel
    // Биты: [size:2][byteAddr:11][bitAddr:3]
    static uint16_t encodeChannel(const DirectAddress& da) {
        uint16_t ch = 0;
        ch |= static_cast<uint16_t>(
            static_cast<uint8_t>(da.size) & 0x03) << 13;
        ch |= static_cast<uint16_t>(da.byteAddr  & 0x07FF) << 3;
        ch |= static_cast<uint16_t>(da.bitAddr   & 0x07);
        return ch;
    }

    // Декодирование для драйвера
    static DirectAddress decodeChannel(uint16_t ch) {
        DirectAddress da{};
        da.size     = static_cast<DirectAddressSize>((ch >> 13) & 0x03);
        da.byteAddr = static_cast<uint16_t>((ch >> 3) & 0x07FF);
        da.bitAddr  = static_cast<uint8_t>(ch & 0x07);
        da.hasBit   = (da.size == DirectAddressSize::BIT);
        return da;
    }

private:
    VirtualMachine&                              vm_;
    std::vector<IOBinding>                       bindings_;
    std::unordered_map<std::string, IIODriver*>  drivers_;
    IOErrorHandler                               errorHandler_;

    // ── Внутренние методы ─────────────────────────────────────

    void resolveVariableIndices() {
        for (auto& binding : bindings_) {
            uint16_t idx = vm_.resolveVarIndex(
                binding.pouName, binding.varName);

            if (idx == VirtualMachine::INVALID_VAR_INDEX)
                throw std::runtime_error(
                    "Cannot resolve variable '" +
                    binding.varName + "' in POU '" +
                    binding.pouName + "'");

            binding.varIndex = idx;
        }
    }

    void writeToVM(const IOBinding& binding, const Value& value) {
        vm_.setVarByIndex(binding.pouName, binding.varIndex, value);
    }

    Value readFromVM(const IOBinding& binding) {
        return vm_.getVarByIndex(binding.pouName, binding.varIndex);
    }

    // Приведение типа значения к ожидаемому типу привязки
    static Value convertType(const Value& v, ValueType target) {
        switch (target) {
            case ValueType::BOOL:  return Value::fromBool (v.asBool());
            case ValueType::INT:   return Value::fromInt  (
                static_cast<int16_t>(v.asInt()));
            case ValueType::DINT:  return Value::fromDInt (v.asInt());
            case ValueType::REAL:  return Value::fromReal (v.asReal());
            default:               return v;
        }
    }

    void handleError(const IOBinding& binding,
                     const std::string& msg)
    {
        if (errorHandler_) {
            errorHandler_(binding, msg);
        }
        // Не бросаем — продолжаем цикл с остальными привязками
    }
};