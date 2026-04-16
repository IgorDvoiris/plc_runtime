#include "ComponentManager.hpp"
#include "SystemBus.hpp"
#include <iostream>

void ComponentManager::registerComponent(IComponent* component) {
    components_.push_back(component);
}

void ComponentManager::initAll(SystemBus& bus) {
    for (auto* c : components_) {
        std::cout << "[ComponentManager] init: " << c->name() << "\n";
        c->init(bus);
    }
}

void ComponentManager::startAll() {
    for (auto* c : components_) {
        std::cout << "[ComponentManager] start: " << c->name() << "\n";
        c->start();
    }
}

void ComponentManager::stopAll() {
    std::cout << "[ComponentManager] stopping all (reverse order)\n";
    for (auto it = components_.rbegin(); it != components_.rend(); ++it) {
        std::cout << "[ComponentManager] stop: " << (*it)->name() << "\n";
        (*it)->stop();
    }
}
