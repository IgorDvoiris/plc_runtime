#pragma once
#include "IComponent.hpp"
#include <vector>

class SystemBus;

class ComponentManager {
public:
    void registerComponent(IComponent* component);
    void initAll(SystemBus& bus);
    void startAll();
    void stopAll();

private:
    std::vector<IComponent*> components_;
};
