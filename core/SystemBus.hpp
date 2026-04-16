#pragma once

#include <typeindex>
#include <unordered_map>
#include <stdexcept>
#include <string>

class SystemBus {
public:
    template<typename TInterface>
    void registerService(TInterface* instance) {
        const auto key = std::type_index(typeid(TInterface));
        if (services_.count(key)) {
            throw std::runtime_error(
                std::string("Service already registered: ") +
                typeid(TInterface).name()
            );
        }
        services_[key] = static_cast<void*>(instance);
    }

    template<typename TInterface>
    TInterface* getService() {
        const auto key = std::type_index(typeid(TInterface));
        auto it = services_.find(key);
        if (it == services_.end()) {
            throw std::runtime_error(
                std::string("Service not found: ") +
                typeid(TInterface).name()
            );
        }
        return static_cast<TInterface*>(it->second);
    }

private:
    std::unordered_map<std::type_index, void*> services_;
};
