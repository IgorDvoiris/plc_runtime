#include "TaskManagerComponent.hpp"
#include "SystemBus.hpp"
#include <iostream>

void TaskManagerComponent::init(SystemBus& bus) {
    scheduler_ = bus.getService<IScheduler>();
    bus.registerService<ITaskManager>(&taskManager_);
    std::cout << "[TaskManagerComponent] ITaskManager registered\n";
}
void TaskManagerComponent::start() {
    taskManager_.bindScheduler(scheduler_);
    std::cout << "[TaskManagerComponent] tasks bound to scheduler\n";
}
void TaskManagerComponent::stop() {
    std::cout << "[TaskManagerComponent] stopped\n";
}
