#include "SchedulerComponent.hpp"
#include "SystemBus.hpp"
#include <iostream>

void SchedulerComponent::init(SystemBus& bus) {
    bus.registerService<IScheduler>(&scheduler_);
    std::cout << "[SchedulerComponent] IScheduler registered\n";
}
void SchedulerComponent::start() { std::cout << "[SchedulerComponent] ready\n"; }
void SchedulerComponent::stop()  { std::cout << "[SchedulerComponent] stopped\n"; }
